#include "core/animation/seb_sprite.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace saga::core;
namespace {
void check_at(bool v,int line){if(!v){std::cerr<<"SEB contract line "<<line<<'\n';std::abort();}}
#define check(v) check_at((v),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
saga::assets::SebRecord record(std::int16_t frame,std::int16_t tex=0,std::int16_t x=0,std::int16_t y=0){
    saga::assets::SebRecord r;r.value_count=10;r.values={frame,tex,3,4,5,7,x,y,0,0};return r;
}
saga::assets::Seb file(std::vector<saga::assets::SebRecord> records){
    const auto count=static_cast<std::uint16_t>(records.size());return {0,10,{{0,count,std::int16_t{0}}},std::move(records)};
}
SpriteArray buffer(){return std::make_shared<SpriteValues>(12,0);}
}
int main(){
    {auto s=prepare_seb_sprites(file({record(0,2,-41,17)}));auto out=buffer();out->at(11)=123;
     check(*s.groups==std::vector<std::int32_t>{0}&&s.packed->size()==8&&s.extended->empty());
     convert_seb_sprite(s,0,out,0);check(*out==SpriteValues({0,2,3,4,5,7,-41,17,0,0,0,123}));
     convert_seb_sprite(s,0,out,1);check(out->at(6)==37&&out->at(8)==1);reverse_seb(s,1,1);convert_seb_sprite(s,0,out,1);
     check(out->at(6)==-41&&out->at(7)==-23&&out->at(8)==0&&out->at(9)==1);}
    {auto r=record(0,300,32767,-32768);r.values[4]=32767;r.values[5]=-3;auto s=prepare_seb_sprites(file({r}));auto out=buffer();
     check(s.packed->at(0)==128&&s.extended->size()==7);reverse_seb(s,1,1);convert_seb_sprite(s,0,out,0);
     check(out->at(1)==300&&out->at(6)==-65533&&out->at(7)==32770&&out->at(11)==-1);}
    {auto s=prepare_seb_sprites(file({record(0,-8,12,-7)}));auto out=buffer();reverse_seb(s,1,1);convert_seb_sprite(s,0,out,0);
     check(out->at(6)==-12&&out->at(7)==7&&out->at(2)==-3&&out->at(3)==-4&&out->at(4)==-5&&out->at(5)==-7);}
    {auto r=record(0,-4);r.values[8]=0x2000;r.values[9]=0x12;auto s=prepare_seb_sprites(file({r}));auto out=buffer();convert_seb_sprite(s,0,out,0);
     check(static_cast<std::uint32_t>(out->at(11))==0x80122000U&&out->at(10)==1&&out->at(8)==1);}
    {saga::assets::SebRecord a;a.value_count=4;a.values={0,0,1,2};a.control=std::uint8_t{1};a.extra_word=0x80abcdefU;a.pair=std::array<std::int16_t,2>{7,8};
     a.tail_pair=std::array<std::int16_t,2>{300,12};a.flags=std::uint8_t{3};auto b=a;b.values={1,-7,3,4};b.control=std::uint8_t{0};b.extra_word.reset();b.pair.reset();b.flags.reset();
     auto source=file({a,b});source.mode=1;auto s=prepare_seb_sprites(source);auto out=buffer();convert_seb_sprite(s,1,out,0);
     check(out->at(1)==-7&&out->at(2)==7&&out->at(3)==8&&out->at(4)==300&&static_cast<std::uint32_t>(out->at(11))==0x80abcdefU);}
    {SebStaticWork work;auto a=record(0,1,10,20),b=record(9,2,13,17);b.values[2]=77;auto s=prepare_seb_sprites(file({a,b}));
     auto out=seb_sprite(s,work,3,0);check(out->at(0)==3&&out->at(1)==1&&out->at(6)==10&&out->at(7)==20&&out->at(2)==3);
     check(seb_sprite(s,work,6,0)==out&&out->at(6)==11&&out->at(7)==19);
     check(seb_sprite(s,work,9,0)==out&&out->at(1)==2&&out->at(2)==77&&out->at(6)==13);
     check(!seb_sprite(s,work,-1,0)&&!seb_sprite(s,work,10,0));
     fails([&]{seb_sprite(s,work,0,-1);});fails([&]{seb_sprite(s,work,0,1);});}
    {SebStaticWork work;auto a=record(0,-8,0,0),b=record(9,-8,9,-9);b.values[2]=99;b.values[3]=88;b.values[4]=77;b.values[5]=66;
     auto s=prepare_seb_sprites(file({a,b}));auto out=seb_sprite(s,work,3,0);
     check(out->at(0)==3&&out->at(6)==3&&out->at(7)==-3&&out->at(2)==3&&out->at(3)==4&&out->at(4)==5&&out->at(5)==7);}
    {SebStaticWork work;auto s=prepare_seb_sprites(file({record(0,1,2,3),record(1,1,4,5),record(2,1,6,7),record(3,1,8,9),record(4,1,10,11)}));
     for(int i=0;i<=4;++i)check(seb_sprite(s,work,i,0)->at(6)==2+i*2);
     check(seb_sprite(s,work,10000,0)->at(6)==-6);s.flip=true;check(seb_sprite(s,work,0,0)->at(8)==1);}
    {SebStaticWork work;auto s=prepare_seb_sprites(file({record(0,1,2,3),record(1,1,4,5),record(2,1,6,7)}));set_seb_cache_size(s,2);
     auto a=seb_sprite(s,work,0,0);auto b=seb_sprite(s,work,1,0);check(a!=b);
     check(seb_sprite(s,work,0,0)==a&&s.cache->at(0)->at(0)==a&&s.cache->at(1)->at(0)==b);
     check(seb_sprite(s,work,10000,0)==a&&a->at(8)==0); // Normalized-key hit ignores raw reverse flag.
     s.flip=true;check(seb_sprite(s,work,0,0)->at(8)==0);reverse_seb(s,1,0);
     check(a->at(0)==-1&&b->at(0)==-1&&seb_sprite(s,work,0,0)->at(8)==0);
     auto old=s.cache;fails([&]{set_seb_cache_size(s,-1);});check(s.cache==old);set_seb_cache_size(s,0);fails([&]{seb_sprite(s,work,0,0);});}
    {auto source=file({record(0,1),record(1,1),record(0,2),record(1,2)});source.groups={{0,2,std::int16_t{0}},{2,2,std::int16_t{0}}};
     auto s=prepare_seb_sprites(source);SebStaticWork work;set_seb_cache_size(s,2);
     auto a=seb_sprite(s,work,0,0);auto other=seb_sprite(s,work,0,1);seb_sprite(s,work,1,0);
     check(s.cache->at(0)->at(1)==other&&s.cache->at(1)->at(0)==a);}
    {SebStaticWork work;auto s=prepare_seb_sprites(file({record(0)}));set_seb_cache_size(s,1);auto cached=seb_sprite(s,work,0,0);
     s.groups.reset();check(seb_sprite(s,work,0,0)==cached);fails([&]{seb_sprite(s,work,1,0);});}
    {SebStaticWork work;auto source=file({record(0,1),record(1,1)});auto a=prepare_seb_sprites(source),b=prepare_seb_sprites(source);
     auto one=seb_sprite_local(a,work,0,0);auto two=seb_sprite_local(b,work,1,0);check(one==two&&one->at(0)==1);
     auto own=seb_sprite(a,work,0,0);check(own!=one);auto list=seb_sprites_local(a,work,0);check(list->at(0)==one);
     check(seb_sprites_local(b,work,1)==list&&list->at(0)->at(0)==1);check(!seb_sprites_local(a,work,4)->at(0));}
    {auto s=prepare_seb_sprites(file({}));SebStaticWork work;check(!seb_sprite(s,work,0,0));
     auto out=buffer();fails([&]{convert_seb_sprite(s,0,out,0);});
     s=prepare_seb_sprites(file({record(0,5)}));auto short_out=std::make_shared<SpriteValues>(2,99);fails([&]{convert_seb_sprite(s,0,short_out,0);});
     check(short_out->at(0)==0&&short_out->at(1)==5);s.groups.reset();fails([&]{seb_sprite(s,work,0,0);});}
}
