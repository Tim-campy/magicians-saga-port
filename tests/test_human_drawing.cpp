#include "core/gameplay/human_drawing.hpp"
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class Exception,class Action>void fails(Action action){bool caught=false;try{action();}catch(const Exception&){caught=true;}check(caught,"expected exception");}
struct Matrix final:RenderMatrix {
    std::array<float,9> values{};
    void set_values(std::array<float,9> v)override{values=v;}
    void pre_translate(float,float)override{throw std::logic_error("unexpected translation");}
    void set_rotation(float,float,float)override{throw std::logic_error("unexpected rotation");}
    void reset()override{throw std::logic_error("unexpected matrix reset");}
};
struct Graphics final:RenderGraphics {
    std::vector<RenderImageDraw> draws;
    std::vector<std::int32_t> reverses;
    int pushes=0,pops=0;
    RenderMatrix* pushed=nullptr;
    std::function<void()> after_draw;
    RenderDepth* mesh_manager()override{throw std::logic_error("unexpected depth");}
    void push_render_mode(std::int32_t,std::int32_t,std::int32_t)override{throw std::logic_error("unexpected render mode");}
    void pop_render_mode()override{throw std::logic_error("unexpected render mode pop");}
    void push_blend_mode(std::int32_t,std::int32_t)override{throw std::logic_error("unexpected blend");}
    void pop_blend_mode()override{throw std::logic_error("unexpected blend pop");}
    void push_matrix(RenderMatrix& value)override{++pushes;pushed=&value;}
    void pop_matrix()override{++pops;}
    void draw_image(const RenderImageDraw& value)override{draws.push_back(value);reverses.push_back(reverse);if(after_draw)after_draw();}
};
saga::assets::Seb source(std::size_t count=5){
    saga::assets::Seb result{0,4,{},{}};
    for(std::size_t i=0;i<count;++i){result.groups.push_back({static_cast<std::uint32_t>(i),1,std::int16_t{0}});
        saga::assets::SebRecord record;record.value_count=10;record.values={0,77,3,4,5,7,2,-3,0,0};result.records.push_back(record);}
    return result;
}
struct Host final:ScaledHumanDrawingHost {
    std::shared_ptr<CharacterProgressionTable> characters=std::make_shared<CharacterProgressionTable>(1);
    std::shared_ptr<EquipmentCombatTable> weapons=std::make_shared<EquipmentCombatTable>(2);
    Matrix matrix;
    SebSpriteState sprite=prepare_seb_sprites(source());
    SharedAnimationClip clock{0,4,&sprite};
    AnimationResource animation{std::make_shared<std::vector<SharedAnimationClip*>>(1,&clock)};
    RenderImage shadow,body,second_body,hat,weapon;
    RenderDrawResource chara{&animation,std::make_shared<std::vector<RenderImage*>>(201,nullptr)};
    RenderDrawResource hats{nullptr,std::make_shared<std::vector<RenderImage*>>(8,nullptr)};
    RenderDrawResource arms{nullptr,std::make_shared<std::vector<RenderImage*>>(14,nullptr)};
    std::vector<std::string> log;
    int character_reads=0,cloth_reads=0;
    std::function<void(const std::string&)> on_call;
    Host(){
        characters->at(0).emplace();characters->at(0)->image=10;characters->at(0)->weapon_id=1;
        weapons->at(1).emplace();weapons->at(1)->image=13;
        chara.images->at(200)=&shadow;chara.images->at(10)=&body;chara.images->at(11)=&second_body;
        hats.images->at(7)=&hat;arms.images->at(13)=&weapon;
    }
    void call(const std::string& name){log.push_back(name);if(on_call)on_call(name);}
    std::shared_ptr<CharacterProgressionTable> human_characters()override{++character_reads;call("character");return characters;}
    std::shared_ptr<EquipmentCombatTable> human_weapons()override{call("weapons");return weapons;}
    std::int32_t hero_cloth_image()override{call("cloth");return ++cloth_reads==1?10:11;}
    std::int32_t hero_hat_image()override{call("hat");return 7;}
    std::int32_t hero_weapon_image()override{call("weapon");return 13;}
    RenderMatrix& human_scale_matrix()override{call("matrix");return matrix;}
    RenderDrawResource* human_character_resource()override{call("source");return &chara;}
    RenderDrawResource* human_resource(std::int32_t id)override{call("resource"+std::to_string(id));
        if(id==1)return &chara;if(id==21)return &hats;if(id==22)return &arms;throw std::logic_error("unexpected human resource");}
};
EventInstruction images(){return std::make_shared<EventIntegers>(EventIntegers{200,10,11,7,13});}
void resolver(){
    Host normal;const auto result=resolve_human_images(0,normal);
    check(*result==EventIntegers({200,10,10,-1,13}),"ordinary images include shadow, body twice, no hat and base weapon");
    check(normal.log==std::vector<std::string>({"character","character","weapons","character","character"}),"ordinary catalog reads and captured equipment owner follow native order");
    Host main;main.characters->at(0)->flags=2;const auto main_images=resolve_human_images(0,main);
    check(*main_images==EventIntegers({200,10,11,7,13})&&main.cloth_reads==2,"main hero rereads cloth independently and supplies its current equipment");
    check(main.log==std::vector<std::string>({"character","character","weapons","character","character","cloth","cloth","hat","weapon"}),"main branch follows all ordinary reads before overrides");
    Host captured;captured.on_call=[&](const auto& name){if(name=="character"&&captured.character_reads==3){
        captured.weapons=std::make_shared<EquipmentCombatTable>(2);captured.weapons->at(1).emplace();captured.weapons->at(1)->image=99;}};
    check(resolve_human_images(0,captured)->at(4)==13,"equipment catalog is captured before fresh character weapon-ID read");
    Host live;live.on_call=[&](const auto& name){if(name=="character"&&live.character_reads==2)live.characters->at(0)->image=11;};
    check(*resolve_human_images(0,live)==EventIntegers({200,10,11,-1,13}),"body image queries remain live instead of aliasing one snapshot");
    Host missing;missing.characters->at(0).reset();fails<std::invalid_argument>([&]{resolve_human_images(0,missing);});
    Host gap;gap.weapons->at(1).reset();fails<std::invalid_argument>([&]{resolve_human_images(0,gap);});
    Host bad;fails<std::out_of_range>([&]{resolve_human_images(-1,bad);});
    Host main_bad;main_bad.characters->at(0)->flags=2;main_bad.characters->at(0)->weapon_id=99;
    fails<std::out_of_range>([&]{resolve_human_images(0,main_bad);});check(main_bad.cloth_reads==0,"invalid base weapon fails before main overrides");
}
void drawing(){
    Host h;Graphics g;SebStaticWork work;HumanDrawingRequest request{100,200,0,4,0,150,0};
    draw_human_images(request,images(),&g,h,work);
    check(g.pushes==1&&g.pops==1&&g.pushed==&h.matrix,"one scale matrix surrounds the complete drawing");
    check(h.matrix.values==std::array<float,9>{1.5F,0,0,0,1.5F,0,0,0,1},"binary32 scale matrix uses scale/100");
    check(g.draws.size()==5&&g.draws[0].image==&h.shadow&&g.draws[1].image==&h.body&&g.draws[2].image==&h.second_body&&g.draws[3].image==&h.hat&&g.draws[4].image==&h.weapon,"all five images route through native resources despite absent TexId77");
    const auto& d=g.draws[0];check(d.x==103&&d.y==195.5F&&!d.scaled&&d.u==3&&d.v==4&&d.source_width==5&&d.source_height==7,"offset products divide as binary32 under matrix; image UV and size remain unscaled");
    check(h.sprite.work->list&&h.log==std::vector<std::string>({"matrix","source","resource1","resource1","resource1","resource21","resource22"}),"instance evaluator and per-layer resource lookup are reused");
    Host wrapped;Graphics target;HumanDrawingRequest large{0,0,0,0,0,std::numeric_limits<std::int32_t>::max(),3};
    draw_human_images(large,images(),&target,wrapped,work);
    check(target.draws.size()==2&&target.draws[0].x==-0.02F,"offset multiplication wraps int32 before float division");
    Host combined;Graphics combined_graphics;draw_scaled_human({100,200,0,0,0,100,0},&combined_graphics,combined,work);
    check(combined_graphics.draws.size()==4&&combined_graphics.draws[3].image==&combined.weapon,"default wrapper resolves ordinary images and skips hat layer");
}
void masks_and_skips(){
    SebStaticWork work;
    for(std::int32_t mask=0;mask<4;++mask){Host h;Graphics g;auto request=HumanDrawingRequest{0,0,0,0,0,100,mask};
        draw_human_images(request,images(),&g,h,work);
        check(g.draws.size()==static_cast<std::size_t>(5-(mask&1?2:0)-(mask&2?1:0)),"mask bits hide equipment layers and shadow independently");}
    Host skipped;Graphics g;auto override=images();override->at(0)=-1;override->at(1)=-2;
    skipped.sprite.cache=std::make_shared<std::vector<SpriteRows>>(1,std::make_shared<std::vector<SpriteArray>>(5));
    for(std::size_t i=0;i<2;++i)skipped.sprite.cache->at(0)->at(i)=std::make_shared<SpriteValues>(SpriteValues{0});
    for(std::size_t i=2;i<5;++i)skipped.sprite.cache->at(0)->at(i)=std::make_shared<SpriteValues>(SpriteValues{0,-1});
    draw_human_images({},override,&g,skipped,work);
    check(g.draws.empty()&&g.pops==1&&skipped.log==std::vector<std::string>({"matrix","source"}),"image-1/-2 skips before sprite field1; only TexId-1 skips before resource lookup");
    Host negative;Graphics failure;auto negative_images=images();negative_images->at(0)=-3;
    fails<std::out_of_range>([&]{draw_human_images({},negative_images,&failure,negative,work);});
    check(failure.pushes==1&&failure.pops==0&&negative.log.back()=="resource1","image-3 reaches the unsigned image bounds failure");
    Host empty;empty.sprite=prepare_seb_sprites(source(0));Graphics empty_graphics;
    draw_human_images({},nullptr,&empty_graphics,empty,work);
    check(empty_graphics.draws.empty()&&empty_graphics.pushes==1&&empty_graphics.pops==1,"empty sprite array does not require texture input but retains matrix scope");
    Host negative_sprite;Graphics negative_graphics;
    negative_sprite.sprite=prepare_seb_sprites(source(1));
    negative_sprite.sprite.cache=std::make_shared<std::vector<SpriteRows>>(1,std::make_shared<std::vector<SpriteArray>>(1,std::make_shared<SpriteValues>(SpriteValues{0,-2,3,4,5,7,2,-3,0,0})));
    draw_human_images({},images(),&negative_graphics,negative_sprite,work);
    check(negative_graphics.draws.size()==1&&negative_graphics.draws[0].image==&negative_sprite.shadow,"sprite TexId-2 still draws the explicit image; only TexId-1 skips");
}
void reversal_and_failures(){
    Host h;Graphics g;SebStaticWork work;g.reverse=7;
    h.sprite.cache=std::make_shared<std::vector<SpriteRows>>(1,std::make_shared<std::vector<SpriteArray>>());
    for(auto flags:std::array<std::array<std::int32_t,2>,5>{{{0,0},{9,0},{0,-7},{1,1},{0,0}}})
        h.sprite.cache->at(0)->push_back(std::make_shared<SpriteValues>(SpriteValues{0,77,3,4,5,7,2,-3,flags[0],flags[1]}));
    draw_human_images({},images(),&g,h,work);
    check(g.reverses==EventIntegers({7,1,2,3,0})&&g.reverse==0,"nonzero flags map to native reversal bits; no-flags preserves current state and flagged draws clear it");
    Host failed;Graphics broken;failed.sprite.cache=h.sprite.cache;broken.after_draw=[&]{throw std::runtime_error("draw");};
    fails<std::runtime_error>([&]{draw_human_images({},images(),&broken,failed,work);});
    check(broken.pushes==1&&broken.pops==0,"draw failure retains pushed matrix");
    Host mirrored;Graphics mirror_failure;
    mirrored.sprite=prepare_seb_sprites(source(1));
    mirrored.sprite.cache=std::make_shared<std::vector<SpriteRows>>(1,std::make_shared<std::vector<SpriteArray>>(1,std::make_shared<SpriteValues>(SpriteValues{0,77,3,4,5,7,2,-3,1,0})));
    mirror_failure.after_draw=[&]{throw std::runtime_error("draw");};
    fails<std::runtime_error>([&]{draw_human_images({},images(),&mirror_failure,mirrored,work);});
    check(mirror_failure.reverse==1&&mirror_failure.pops==0,"failed mirrored draw retains its native reverse state");
    Host late;Graphics partial;auto selected=images();partial.after_draw=[&]{if(partial.draws.size()==1){selected->at(1)=-2;late.sprite.work->list->at(1).reset();}};
    draw_human_images({},selected,&partial,late,work);check(partial.draws.size()==4,"later explicit texture values are read after earlier draw callbacks");
    Host absent;Graphics target;fails<std::invalid_argument>([&]{draw_human_images({},nullptr,&target,absent,work);});
    check(target.pushes==1&&target.pops==0,"missing override array fails after SEB preparation and matrix push");
    Host null_graphics;fails<std::invalid_argument>([&]{draw_human_images({},images(),nullptr,null_graphics,work);});
    check(null_graphics.log==std::vector<std::string>({"matrix"}),"missing graphics fails before resource queries");
}
}
int main(){
    const char* stage="resolver";
    try{resolver();stage="drawing";drawing();stage="masks";masks_and_skips();stage="reversal";reversal_and_failures();return 0;}
    catch(const std::exception& error){std::cerr<<stage<<": "<<error.what()<<'\n';return 1;}
}
