#include "animation_resources.hpp"
#include "combat_rules.hpp"
#include "presentation_text.hpp"
#include <algorithm>
#include <limits>
#include <unordered_set>

namespace saga::core {
namespace {
std::int32_t motion_word(std::uint32_t value){
    return static_cast<std::int32_t>(value<=0x7fffffffU?static_cast<std::int64_t>(value):
                                    static_cast<std::int64_t>(value)-0x100000000LL);
}
std::int32_t motion_subtract(std::int32_t a,std::int32_t b){
    return motion_word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));
}
std::int32_t motion_integer(float value){
    if(value==std::numeric_limits<float>::infinity())return std::numeric_limits<std::int32_t>::min();
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    if(value<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(value!=value)return 0;
    return static_cast<std::int32_t>(value);
}
std::vector<const assets::Seb*> selected_files(const ResourceCatalogIndex& index,const std::vector<const assets::Seb*>& files) {
    std::vector<const assets::Seb*> result(index.rows.size(),nullptr);
    for(std::size_t slot=0;slot<index.rows.size();++slot)
        if(index.rows[slot])result[slot]=files.at(*index.rows[slot]);
    return result;
}
}
std::int32_t image_animation_cubic_in_out(std::int32_t start,std::int32_t end,
                                         std::int32_t duration,std::int32_t progress){
    if(progress<0)return start;
    if(progress>=duration)return end;
    // Penner's public cubicInOut, also published by d3-ease v3.0.1/src/cubic.js
    // (BSD-3-Clause; see third_party/licenses/d3-ease.txt). The game wrapper
    // divides by half-duration and preserves these binary32 operation boundaries.
    const volatile float half_duration=static_cast<float>(duration)*0.5F;
    const volatile float t=static_cast<float>(progress)/half_duration;
    const auto difference=motion_subtract(end,start);
    const volatile float half_change=static_cast<float>(difference)*0.5F;
    float result;
    if(t<1.0F){
        const volatile float first=half_change*t;
        const volatile float second=t*first;
        const volatile float third=t*second;
        const volatile float value=third+static_cast<float>(start);
        result=value;
    }else{
        const volatile float shifted=t-2.0F;
        const volatile float square=shifted*shifted;
        const volatile float cube=shifted*square;
        const volatile float curve=cube+2.0F;
        const volatile float delta=half_change*curve;
        const volatile float value=delta+static_cast<float>(start);
        result=value;
    }
    return std::clamp(motion_integer(result),std::min(start,end),std::max(start,end));
}
const ImageDrawingPosition* update_image_animation(const ImageAnimationGroup* group,
    std::int32_t image,ImageDrawingPosition& work){
    if(!group)return nullptr;
    const auto pair=group->at(static_cast<std::size_t>(image));
    if(!pair)throw std::invalid_argument("missing image animation pair");
    const auto mode=pair->at(0),frame=pair->at(1);
    if(mode==1){
        // ResourceManagerExtension.PopOut 0x10ee670. Phase selection uses the
        // wrapped frame-2, even for the last phase's separate frame-11 progress.
        const auto phase=motion_subtract(frame,2);
        std::int32_t scale;
        if(phase<=4)scale=easing(60,110,5,phase,80);
        else if(phase<=6)scale=easing(110,95,2,motion_subtract(frame,7),80);
        else if(phase<=8)scale=easing(95,106,2,motion_subtract(frame,9),80);
        else scale=easing(106,100,7,motion_subtract(frame,11),0);
        work={0,0,scale,scale,0};
    }else if(mode==2){
        // ResourceManagerExtension.SwingUpDown 0x10ee754. Signed remainder is
        // retained for negative clocks; the second half reflects around 11.
        const auto phase=motion_subtract(frame,5)%9;
        const auto progress=phase>5?motion_subtract(11,phase):phase;
        const auto y=image_animation_cubic_in_out(0,3,5,progress);
        work={0,y,100,100,0};
    }else return nullptr;
    return &work;
}
ResourceCatalogIndex index_resource_catalog(std::vector<std::int32_t> ids,std::size_t max_slots) {
    if(ids.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("resource catalog row count exceeds int32");
    auto length=static_cast<std::int32_t>(ids.size());
    for(const auto id:ids) {
        const auto next=id==std::numeric_limits<std::int32_t>::max()?std::numeric_limits<std::int32_t>::min():id+1;
        if(next>length)length=next;
    }
    if(static_cast<std::size_t>(length)>max_slots)throw std::length_error("resource catalog slot budget exceeded");
    std::unordered_set<std::int32_t> used(ids.begin(),ids.end());
    std::int32_t candidate=0; // The used set only grows, so the smallest free ID never decreases.
    for(auto& id:ids)if(id==-1) {
        while(used.find(candidate)!=used.end())++candidate;
        id=candidate;used.insert(candidate);
    }
    ResourceCatalogIndex result{std::move(ids),std::vector<std::optional<std::size_t>>(static_cast<std::size_t>(length))};
    for(std::size_t row=0;row<result.ids.size();++row)
        result.rows.at(static_cast<std::size_t>(result.ids[row]))=row;
    return result;
}
AnimationResource* get_animation_resource(const ApplicationAnimationResources& r,std::int32_t id) noexcept {
    return get_application_resource(r,id);
}
std::shared_ptr<ImageAnimationGroup> load_image_animation_infos(std::size_t count,CatalogRows rows,ImageAnimationTextHost& host){
    if(count>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("image animation count exceeds int32");
    auto result=std::make_shared<ImageAnimationGroup>(count);
    for(auto& row:*result)row=std::make_shared<EventIntegers>(2,0);
    if(!rows)throw std::invalid_argument("null image animation catalog");
    for(std::size_t index=0;index<rows->size();++index){
        const auto fields=split_trimmed_text_fields(rows->at(index).get(),u',');
        for(std::size_t field=1;field<fields.size();++field){
            const auto option=host.lower_case(fields[field]);
            if(!option)throw std::invalid_argument("null lowercase image option");
            if(!host.starts_with(*option,u"a"))continue;
            const auto suffix=option->substr(1);const auto pair=result->at(index);
            const auto mode=host.parse_id(suffix);
            if(!pair)throw std::invalid_argument("null image animation row");
            pair->at(0)=mode;break;
        }
    }
    return result;
}
std::int32_t animation_clip_max_frame(const ApplicationAnimationResources& app,std::int32_t resource,std::int32_t clip) {
    auto* r=get_animation_resource(app,resource);
    if(!r||!r->clips)throw std::invalid_argument("missing effect animation resource");
    auto* value=r->clips->at(static_cast<std::size_t>(clip));
    if(!value)throw std::invalid_argument("missing effect animation clip");
    return value->max_frame;
}
PreparedAnimationResource::PreparedAnimationResource(const std::vector<const assets::Seb*>& files)
    :clocks_(files.size()),sprites_(files.size()),resource_{std::make_shared<std::vector<SharedAnimationClip*>>(files.size(),nullptr)} {
    SebStaticWork loading_work;
    for(std::size_t i=0;i<files.size();++i)if(files[i]) {
        // _load first calls Dispose (frame=0), then stores the signed header short at +0x1c.
        sprites_[i]=prepare_seb_sprites(*files[i]);
        prepare_seb_bounds(*sprites_[i],loading_work,files[i]->header_value);
        clocks_[i]=SharedAnimationClip{0,files[i]->header_value,&*sprites_[i]};
        resource_.clips->at(i)=&*clocks_[i];
    }
}
PreparedAnimationResource::PreparedAnimationResource(const ResourceCatalogIndex& index,const std::vector<const assets::Seb*>& files)
    :PreparedAnimationResource(selected_files(index,files)) {}
} // namespace saga::core
