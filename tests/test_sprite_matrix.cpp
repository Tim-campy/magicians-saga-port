#include "core/gameplay/sprite_matrix.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"sprite matrix contract line "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
MatrixValues values(std::initializer_list<float> v){return std::make_shared<std::vector<float>>(v);}
bool close(float a,float b){return std::abs(a-b)<0.00001F;}
int main(){
    {SpriteMatrix m;auto identity=m.values;set_matrix_values(m,values({2,3}));check(m.values==identity&&m.values->at(0)==2&&m.values->at(1)==3&&m.values->at(4)==1);
     m.values->resize(10,77);check(&reset_matrix(m)==&m&&m.values->at(9)==77);m.values->resize(2);fails([&]{reset_matrix(m);});check(*m.values==std::vector<float>({1,0}));}
    {SpriteMatrix m,n;n.values=values({2,3});fails([&]{copy_matrix(m,&n);});check(m.values->at(0)==1);m.values=values({7});copy_matrix(m,&n);check(*m.values==std::vector<float>({2}));}
    {SpriteMatrix m,a,b;set_translate_matrix(a,3,4);fails([&]{set_concat_matrix(m,&a,nullptr);});check(m.values->at(6)==0);
     b.values.reset();fails([&]{set_concat_matrix(m,&a,&b);});check(m.values->at(6)==3&&m.values->at(7)==4);}
    {SpriteMatrix m;m.values=values({1,2,3,4,5,6,7,8,9});auto b=values({10,11,12,13,14,15,16,17,18});
     post_concat_matrix(m,b);check(*m.values==std::vector<float>({84,90,96,201,216,231,318,342,366}));auto scratch=m.concat_work;
     m.values=values({1,2,3,4,5,6,7,8,9});pre_concat_matrix(m,b);check(m.concat_work==scratch&&*m.values==std::vector<float>({138,171,204,174,216,258,210,261,312}));}
    {SpriteMatrix m;m.values=values({1,2,3,4,5,6,7,8,9});post_concat_matrix(m,m.values);check(*m.values==std::vector<float>({30,36,42,66,81,96,102,126,150}));
     m.concat_work=values({77,88});fails([&]{post_concat_matrix(m,values({2}));});check(m.concat_work->at(0)==60&&m.concat_work->at(1)==0);}
    {SpriteMatrix m;pre_translate_matrix(m,10,20);pre_scale_matrix(m,2,3,0,0);float x=1,y=2;map_matrix_point(m,x,y);check(x==12&&y==26);
     reset_matrix(m);pre_translate_matrix(m,10,20);post_scale_matrix(m,2,3,0,0);x=1;y=2;map_matrix_point(m,x,y);check(x==22&&y==66);}
    {SpriteMatrix m;set_scale_matrix(m,-1,1,4,5);float x=2,y=3;map_matrix_point(m,x,y);check(x==6&&y==3);
     set_translate_matrix(m,2,3);pre_scale_matrix(m,2,3,4,5);x=4;y=5;map_matrix_point(m,x,y);check(x==6&&y==8);}
    {SpriteMatrix m;m.values=values({1,2,3,4,5,6,7,8,9});float x=2,y=3;map_matrix_point(m,x,y);check(x==21&&y==27);
     m.values->resize(7);x=2;y=3;fails([&]{map_matrix_point(m,x,y);});check(x==21&&y==3);}
    {SpriteMatrix m;m.values=values({1,2,3});fails([&]{post_translate_matrix(m,2,4);});check(*m.values==std::vector<float>({7,14,3}));
     m.values=values({1,2,3,4,5,6,7});fails([&]{pre_translate_matrix(m,2,4);});check(m.values->at(6)==25);}
    {SpriteMatrix m;const auto nan=std::numeric_limits<float>::quiet_NaN();pre_translate_matrix(m,2,3);auto before=*m.values;
     fails([&]{post_scale_matrix(m,nan,1,2,3);});check(*m.values==before);fails([&]{set_rotate_matrix(m,nan,0,0);});check(m.values->at(6)==0);
     post_translate_matrix(m,std::numeric_limits<float>::infinity(),0);check(std::isnan(m.values->at(0))&&std::isinf(m.values->at(6)));}
    {SpriteMatrix m;set_rotate_matrix(m,90,4,5);float x=6,y=5;map_matrix_point(m,x,y);check(close(x,4)&&close(y,7));
     reset_matrix(m);pre_translate_matrix(m,10,0);pre_rotate_matrix(m,90,0,0);x=2;y=0;map_matrix_point(m,x,y);check(close(x,10)&&close(y,2));
     check(degree_sine(90)==1&&degree_cosine(0)==1&&close(degree_tangent(45),1));
     check(degree_tangent(std::numeric_limits<float>::infinity())==0&&degree_tangent(std::numeric_limits<float>::quiet_NaN())==0);}
    std::cout<<"sprite matrix contract ok\n";
}
