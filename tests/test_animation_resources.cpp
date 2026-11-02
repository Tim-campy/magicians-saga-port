#include "core/gameplay/animation_resources.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class E,class F>void fails(F f){bool caught=false;try{f();}catch(const E&){caught=true;}check(caught);}
struct ImageText final:ImageAnimationTextHost {
    std::vector<std::u16string> parsed;
    CatalogText lower_case(const std::u16string& s)override{auto value=s;for(auto& c:value)if(c>=u'A'&&c<=u'Z')c+=u'a'-u'A';return std::make_shared<const std::u16string>(value);}
    bool starts_with(const std::u16string& s,std::u16string_view prefix)override{return s.compare(0,prefix.size(),prefix)==0;}
    std::int32_t parse_id(std::u16string_view s)override{parsed.emplace_back(s);if(s==u"1")return 1;if(s==u"2")return 2;throw std::invalid_argument("mode");}
};
void position_is(const ImageDrawingPosition& value,std::int32_t x,std::int32_t y,
                 std::int32_t scale_x,std::int32_t scale_y,std::int32_t angle){
    check(value.x==x&&value.y==y&&value.scale_x==scale_x&&value.scale_y==scale_y&&value.angle==angle);
}
void image_motion(){
    const auto minimum=std::numeric_limits<std::int32_t>::min(),maximum=std::numeric_limits<std::int32_t>::max();
    // At quarter, half and three-quarter time, the normalized public cubic is
    // exactly 1/16, 1/2 and 15/16. These values are exactly representable in binary32.
    check(image_animation_cubic_in_out(0,128,4,1)==8);
    check(image_animation_cubic_in_out(0,128,4,2)==64);
    check(image_animation_cubic_in_out(0,128,4,3)==120);
    check(image_animation_cubic_in_out(128,0,4,1)==120);
    check(image_animation_cubic_in_out(128,0,4,2)==64);
    check(image_animation_cubic_in_out(128,0,4,3)==8);
    // Swing's 0..3 output crosses integer boundaries only at the last three samples.
    const std::array<std::int32_t,6> cubic{0,0,0,2,2,3};
    for(std::int32_t progress=0;progress<=5;++progress)
        check(image_animation_cubic_in_out(0,3,5,progress)==cubic.at(static_cast<std::size_t>(progress)));
    check(image_animation_cubic_in_out(9,17,5,-1)==9);
    check(image_animation_cubic_in_out(9,17,5,maximum)==17);
    check(image_animation_cubic_in_out(9,17,0,-1)==9);
    check(image_animation_cubic_in_out(9,17,0,0)==17);
    check(image_animation_cubic_in_out(9,17,-10,-3)==9);
    check(image_animation_cubic_in_out(9,17,-10,0)==17);
    check(image_animation_cubic_in_out(0,maximum,4,2)==1073741824); // binary32 rounds maximum upward before halving.
    check(image_animation_cubic_in_out(-2000000000,2000000000,4,2)==-2000000000); // Wrapped delta, then endpoint clamp.
    check(image_animation_cubic_in_out(2000000000,-2000000000,4,2)==2000000000);
    check(image_animation_cubic_in_out(minimum,maximum,4,2)==minimum);
    check(image_animation_cubic_in_out(maximum,minimum,4,2)==maximum);

    ImageDrawingPosition work{11,12,13,14,15};
    position_is(ImageDrawingPosition{},0,0,100,100,0);
    check(!update_image_animation(nullptr,minimum,work));position_is(work,11,12,13,14,15);
    ImageAnimationGroup group{std::make_shared<EventIntegers>(EventIntegers{1,0}),
                              std::make_shared<EventIntegers>(EventIntegers{2,0})};
    // PopOut phase boundaries, the overshoot/undershoot samples, and the complete linear tail.
    const std::array<std::int32_t,19> pop{60,60,60,77,92,102,107,110,98,95,103,106,105,104,103,102,101,100,100};
    const ImageDrawingPosition* shared=nullptr;
    for(std::int32_t frame=0;frame<static_cast<std::int32_t>(pop.size());++frame){
        group[0]->at(1)=frame;shared=update_image_animation(&group,0,work);
        check(shared==&work);const auto scale=pop.at(static_cast<std::size_t>(frame));position_is(work,0,0,scale,scale,0);
        check(group[0]->at(0)==1&&group[0]->at(1)==frame);
    }
    for(const auto frame:{-100,-1,minimum+2}){
        group[0]->at(1)=frame;check(update_image_animation(&group,0,work)==shared);position_is(work,0,0,60,60,0);
    }
    for(const auto frame:{minimum,minimum+1,maximum}){
        group[0]->at(1)=frame;check(update_image_animation(&group,0,work)==shared);position_is(work,0,0,100,100,0);
    }
    // Signed remainder keeps ordinary negative clocks at zero; the wrapped subtraction
    // sends the first three int32 clocks through the positive end of the nine-tick wave.
    const std::array<std::int32_t,9> swing{0,0,0,2,2,3,3,2,2};
    for(std::int32_t frame=0;frame<23;++frame){
        group[1]->at(1)=frame;check(update_image_animation(&group,1,work)==shared);
        const auto y=frame<5?0:swing.at(static_cast<std::size_t>((frame-5)%9));position_is(*shared,0,y,100,100,0);
        check(group[1]->at(0)==2&&group[1]->at(1)==frame);
    }
    for(const auto frame:{-100,-4,-1,minimum+5}){
        group[1]->at(1)=frame;update_image_animation(&group,1,work);position_is(work,0,0,100,100,0);
    }
    const std::array<std::array<std::int32_t,2>,4> extremes{{{minimum,3},{minimum+1,2},{minimum+2,2},{maximum,3}}};
    for(const auto& item:extremes){
        group[1]->at(1)=item[0];update_image_animation(&group,1,work);position_is(work,0,item[1],100,100,0);
    }
    for(const auto mode:{minimum,-1,0,3,maximum}){
        work={11,12,13,14,15};group[0]->at(0)=mode;group[0]->at(1)=maximum;
        check(!update_image_animation(&group,0,work));position_is(work,11,12,13,14,15);
        check(group[0]->at(0)==mode&&group[0]->at(1)==maximum);
    }
    fails<std::out_of_range>([&]{update_image_animation(&group,-1,work);});
    fails<std::out_of_range>([&]{update_image_animation(&group,2,work);});
    group[0].reset();fails<std::invalid_argument>([&]{update_image_animation(&group,0,work);});
    for(const auto mode:{0,1,2,3}){
        group[0]=std::make_shared<EventIntegers>(1,mode);
        fails<std::out_of_range>([&]{update_image_animation(&group,0,work);});position_is(work,11,12,13,14,15);
    }
    group[0]->clear();fails<std::out_of_range>([&]{update_image_animation(&group,0,work);});
    group.clear();fails<std::out_of_range>([&]{update_image_animation(&group,0,work);});
    position_is(work,11,12,13,14,15);
}
int main(){
    image_motion();
    {ImageText host;auto rows=std::make_shared<std::vector<CatalogText>>();
     for(auto s:{u"file.png, A1 ,a2",u"other.png,q+1,a2",u"plain.png"})rows->push_back(std::make_shared<const std::u16string>(s));
     auto group=load_image_animation_infos(5,rows,host);
     check(group->size()==5&&group->at(0)->at(0)==1&&group->at(1)->at(0)==2&&group->at(2)->at(0)==0&&group->at(4)->at(0)==0);
     check(host.parsed==std::vector<std::u16string>{u"1",u"2"}&&group->at(0)!=group->at(1));
     for(const auto& row:*group)check(row->size()==2&&row->at(1)==0);
     fails<std::invalid_argument>([&]{load_image_animation_infos(0,nullptr,host);});
     rows->assign(1,std::make_shared<const std::u16string>(u"file.png,a"));
     fails<std::invalid_argument>([&]{load_image_animation_infos(1,rows,host);});
     const auto parsed=host.parsed.size();
     fails<std::out_of_range>([&]{load_image_animation_infos(0,rows,host);});check(host.parsed.size()==parsed);
     rows->at(0)=std::make_shared<const std::u16string>(u"file.png,a2");
     fails<std::out_of_range>([&]{load_image_animation_infos(0,rows,host);});check(host.parsed.size()==parsed);
     rows->at(0)=nullptr;check(load_image_animation_infos(0,rows,host)->empty());}
    {const auto c=index_resource_catalog({2,-1,2,-1,8,-1});
     check(c.ids==std::vector<std::int32_t>{2,0,2,1,8,3}&&c.rows.size()==9);
     check(c.rows[0]==1&&c.rows[1]==3&&c.rows[2]==2&&c.rows[3]==5&&c.rows[8]==4);
     check(!c.rows[4]&&!c.rows[7]);}
    {const auto c=index_resource_catalog({0,0,0});check(c.rows.size()==3&&c.rows[0]==2&&!c.rows[1]&&!c.rows[2]);}
    check(index_resource_catalog({}).rows.empty());
    check(index_resource_catalog({-1,-1,-1}).ids==std::vector<std::int32_t>{0,1,2});
    fails<std::out_of_range>([]{index_resource_catalog({-2});});
    fails<std::out_of_range>([]{index_resource_catalog({std::numeric_limits<std::int32_t>::max()});});
    fails<std::length_error>([]{index_resource_catalog({8},8);});
    AnimationResource r[13];ApplicationAnimationResources app;
    app.characters=&r[0];app.common=&r[1];app.enemies=&r[2];app.equipment=&r[3];
    app.quest=&r[4];app.residents=&r[5];app.selection=&r[6];app.chips=&r[7];app.bullets=&r[8];
    app.effects=&r[9];app.hats=&r[10];app.weapons=&r[11];app.gauges=&r[12];
    const std::vector<int> supported{1,2,3,4,7,8,11,18,19,20,21,22,23};
    std::size_t found=0;
    for(int id=0;id<24;++id){auto* result=get_animation_resource(app,id);
        if(found<supported.size()&&id==supported[found])check(result==&r[found++]);else check(!result);}
    check(found==13&&!get_animation_resource(app,-1)&&!get_animation_resource(app,24));
    check(!get_animation_resource(app,std::numeric_limits<std::int32_t>::min()));
    check(!get_animation_resource(app,std::numeric_limits<std::int32_t>::max()));
    saga::assets::Seb one{0,13,{},{}},two{1,0,{},{}};
    {const auto indexed=index_resource_catalog({2,0,2,-1});
     PreparedAnimationResource catalog(indexed,{&one,&two,&two,&one});
     const auto& slots=*catalog.resource().clips;
     check(slots.size()==4&&slots[0]->max_frame==0&&slots[1]->max_frame==13&&slots[2]->max_frame==0&&!slots[3]);
     fails<std::out_of_range>([&]{PreparedAnimationResource bad(indexed,{});});}
    PreparedAnimationResource prepared({&one,nullptr,&two,&one});app.effects=&prepared.resource();
    check(animation_clip_max_frame(app,20,0)==13&&animation_clip_max_frame(app,20,2)==0);
    check(prepared.resource().clips->at(0)->frame==0&&!prepared.resource().clips->at(1));
    check(prepared.resource().clips->at(0)!=prepared.resource().clips->at(3)); // Same file, separate loaded instances.
    advance_shared_animation(*prepared.resource().clips->at(0),{false,true});
    check(prepared.resource().clips->at(0)->frame==1&&prepared.resource().clips->at(3)->frame==0);
    one.header_value=9;check(animation_clip_max_frame(app,20,0)==13); // Storage owns scalar clocks.
    fails<std::invalid_argument>([&]{animation_clip_max_frame(app,0,0);});
    fails<std::invalid_argument>([&]{animation_clip_max_frame(app,20,1);});
    fails<std::out_of_range>([&]{animation_clip_max_frame(app,20,-1);});
    fails<std::out_of_range>([&]{animation_clip_max_frame(app,20,4);});
}
