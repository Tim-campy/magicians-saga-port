#include "gauge_frame.hpp"
#include "ecs_combat.hpp"
#include "select_camera.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing gauge frame input");return *p;}
template<class T>T* component(ComponentSlots& e,std::int32_t slot){
    auto* raw=e.get(slot);auto* value=dynamic_cast<T*>(raw);
    if(raw&&!value)throw std::invalid_argument("incorrect gauge component type");return value;
}
EntityGauge& gauge(ComponentSlots& e){return required(component<EntityGauge>(e,19));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
ImageArrays& image(ComponentSlots& e){return required(component<EntityImage>(e,7)).arrays;}
void store(const ImageArray& a,std::size_t at,std::int32_t value){required(a.get()).at(at)=value;}
}
GaugeOffsets initial_gauge_offsets(){
    auto result=std::make_shared<std::vector<EventInstruction>>();
    for(const auto& row:std::array<std::array<std::int32_t,3>,8>{{
        {{-50,0,-20}},{{-50,0,-50}},{{-50,0,-50}},{{-75,0,-50}},
        {{-60,0,-35}},{{-75,0,-50}},{{-30,0,-30}},{{-75,0,-50}}}})
        result->push_back(std::make_shared<EventIntegers>(row.begin(),row.end()));
    return result;
}
ComponentSlots& add_gauge(ComponentSlots& e,GaugeCreation args,GaugePool& pool){
    auto& g=pool.acquire_gauge();g.type=args.type;g.from=args.from;g.to=args.to;g.maximum=args.maximum;
    g.frame=0;g.frame_max=args.frame_max;g.flags=args.flags;g.option=args.option;g.option2=args.option2;
    const auto table=pool.gauge_offsets();const auto row=required(table.get()).at(static_cast<std::size_t>(args.type));
    auto& offsets=required(row.get());g.offset_x=offsets.at(0);g.offset_y=offsets.at(1);g.offset_z=offsets.at(2);
    g.parent=args.parent;return e.add(19,&g);
}
ComponentSlots& create_gauge(GaugeCreation args,EntityIdState& ids,GaugeFactoryHost& host,
                            GaugeDrawState& scratch,GaugeDrawingHost& images){
    auto& parent=required(args.parent);const auto x=required(component<ContactPosition>(parent,0)).x;
    auto table=host.gauge_offsets();auto row=required(table.get()).at(static_cast<std::size_t>(args.type));
    const auto offset_x=required(row.get()).at(0);
    const auto z=required(component<ContactPosition>(parent,0)).z;
    table=host.gauge_offsets();row=required(table.get()).at(static_cast<std::size_t>(args.type));
    const auto offset_z=required(row.get()).at(1); // Native wrapper uses the Y entry for initial Z.
    const auto xi=required(component<CellComponent>(parent,5)).xi;
    const auto yi=required(component<CellComponent>(parent,5)).yi;
    required(host.gauge_map());required(host.gauge_map()); // Queried twice; inner stepX/stepZ arguments are unused.
    GaugePlacement placement;placement.x=add(factory_cell_coordinate(x,1),offset_x);placement.z=add(factory_cell_coordinate(z,1),offset_z);
    placement.xi=xi;placement.yi=yi;return create_gauge_at(args,placement,ids,host,scratch,images);
}
ComponentSlots& create_gauge_at(GaugeCreation args,GaugePlacement placement,EntityIdState& ids,GaugeFactoryHost& host,
                               GaugeDrawState& scratch,GaugeDrawingHost& images){
    // Native step_x/step_z inputs are unused. No Speed or Animation component.
    auto& e=create_entity(ids,0,false,host);add_gauge(e,args,host);
    auto& position=host.acquire_position();position.x=static_cast<float>(placement.x);position.y=0;position.z=static_cast<float>(placement.z);
    position.offset_x=0;position.offset_y=0;position.offset_z=0;position.parent=nullptr;e.add(0,&position);
    auto& sprite=host.acquire_sprite();sprite.animation={placement.resource,placement.clip,0};sprite.layers=std::make_shared<EventIntegers>(1,-1);e.add(2,&sprite);
    ImageArrays arrays;arrays.textures=std::make_shared<EventIntegers>(1,args.image);arrays.resources=std::make_shared<EventIntegers>(1,placement.resource);
    arrays.u=std::make_shared<EventIntegers>(2,0);arrays.v=std::make_shared<EventIntegers>(2,0);
    arrays.width=std::make_shared<EventIntegers>(2,0);arrays.height=std::make_shared<EventIntegers>(2,0);
    add_image_arrays(e,std::move(arrays),host);host.add_cell(e,placement.xi,placement.yi);
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=10000;e.add(4,&depth);
    update_gauge_display(&e,scratch,images);return e;
}
void initialize_gauges(GaugeFrameState& state,GaugeInitializationWorld* world){
    state.subset=required(world).gauge_subset({19,7,2,0,5});
    state.pending=std::make_shared<std::vector<ComponentSlots*>>();
}
void update_gauge_display(ComponentSlots* value,GaugeDrawState& scratch,GaugeDrawingHost& host){
    auto& e=required(value);auto* target=component<ContactPosition>(e,0);
    const auto x=required(component<ContactPosition>(required(gauge(e).parent),0)).x;
    required(target).x=static_cast<float>(add(factory_cell_coordinate(x,1),gauge(e).offset_x));
    target=component<ContactPosition>(e,0);
    const auto y=required(component<ContactPosition>(required(gauge(e).parent),0)).y;
    required(target).y=static_cast<float>(add(factory_cell_coordinate(y,1),gauge(e).offset_y));
    target=component<ContactPosition>(e,0);
    const auto z=required(component<ContactPosition>(required(gauge(e).parent),0)).z;
    required(target).z=static_cast<float>(add(factory_cell_coordinate(z,1),gauge(e).offset_z));
    auto* destination=component<CellComponent>(e,5);
    const auto xi=required(component<CellComponent>(required(gauge(e).parent),5)).xi;required(destination).xi=xi;
    destination=component<CellComponent>(e,5);
    const auto yi=required(component<CellComponent>(required(gauge(e).parent),5)).yi;required(destination).yi=yi;
    float ratio=0.0F;
    if((gauge(e).flags&3)!=0){
        auto& parent=required(gauge(e).parent);
        if(parent.contains(23)){
            const auto& values=required(required(component<CombatParameterLinks>(parent,23)).values);
            const auto found=values.find(gauge(e).option); // ParameterComponent.Get -> nullable ParamSet.Get.
            if(found!=values.end()&&found->second){
                const auto& p=*found->second;
                if(gauge(e).type==4)ratio=static_cast<float>(p.experience)/static_cast<float>(p.max_experience);
                else ratio=static_cast<float>(add(p.value,p.extra))/static_cast<float>(add(p.maximum,p.extra_maximum));
            }
        }
    }else{
        const auto frame=gauge(e).frame,maximum_frame=gauge(e).frame_max;
        const auto to=gauge(e).to,from=gauge(e).from;
        const auto origin=gauge(e).from,candidate_to=gauge(e).to,candidate_maximum=gauge(e).maximum;
        const float fraction=static_cast<float>(frame)/static_cast<float>(sub(maximum_frame,1));
        const float interpolated=fraction*static_cast<float>(sub(to,from));
        const auto delta=factory_cell_coordinate(interpolated,1);
        const auto upper=candidate_to<candidate_maximum?gauge(e).to:gauge(e).maximum;
        const auto lower=gauge(e).from;
        const auto bounded=camera_clamp(add(origin,delta),lower,upper);
        ratio=static_cast<float>(bounded)/static_cast<float>(gauge(e).maximum);
    }
    const auto texture=required(image(e).textures.get()).at(0);
    auto* source=host.gauge_image(texture); // Always AppData resource23, irrespective of image resource overrides.
    const auto offset=gauge_image_offset(scratch,source,host);
    store(image(e).u,0,0);store(image(e).u,1,offset);
    store(image(e).v,0,0);store(image(e).v,1,required(source).height/2);
    store(image(e).width,0,source->width);
    const auto usable=sub(source->width,add(offset,offset));
    store(image(e).width,1,factory_cell_coordinate(ratio*static_cast<float>(usable),1));
    store(image(e).height,0,source->height/2);store(image(e).height,1,source->height/2);
}
void update_gauges(GaugeFrameState& state,GaugeDrawState& scratch,GaugeFrameHost& host,GaugeDrawingHost& images){
    {
        auto cursor=required(state.subset).cursor();
        while(const auto entry=cursor.next()){
            auto& e=required(*entry);auto* parent=gauge(e).parent;
            if(!parent||host.gauge_parent_destroyed(*parent)){required(state.pending.get()).push_back(&e);continue;}
            if(gauge(e).frame_max!=-1){
                auto& frame=gauge(e).frame;frame=add(frame,1);
                if(gauge(e).frame>add(gauge(e).frame_max,15))required(state.pending.get()).push_back(&e);
            }
            update_gauge_display(&e,scratch,images); // Expiry does not skip the native geometry suffix.
        }
    }
    for(;;){
        auto& pending=required(state.pending.get());if(pending.empty())return;auto* e=pending.at(0);
        host.destroy_gauge(required(e),true);auto& current=required(state.pending.get());
        if(current.empty())throw std::out_of_range("gauge removal queue was cleared by callback");
        current.erase(current.begin());
    }
}
} // namespace saga::core
