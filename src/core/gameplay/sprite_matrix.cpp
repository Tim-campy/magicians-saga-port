#include "sprite_matrix.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing sprite matrix input");return *p;}
float& at(const MatrixValues& v,std::size_t i){return required(v.get()).at(i);}
void copy_values(const MatrixValues& from,const MatrixValues& to,std::size_t count){
    auto& source=required(from.get());auto& destination=required(to.get());
    if(count>source.size()||count>destination.size())throw std::out_of_range("sprite matrix copy range");
    if(from!=to)std::copy_n(source.begin(),count,destination.begin());
}
void number(float value){if(std::isnan(value))throw std::invalid_argument("NaN sprite matrix parameter");}
SpriteMatrix& concat(SpriteMatrix& m,MatrixValues other,bool pre){
    required(other.get());
    if(!m.concat_work)m.concat_work=std::make_shared<std::vector<float>>(9);
    auto work=m.concat_work;std::fill(work->begin(),work->end(),0.0F);
    for(std::size_t column=0;column<3;++column)for(std::size_t row=0;row<3;++row){
        auto& output=work->at(column*3+row);output=0.0F;float sum=0.0F;
        for(std::size_t k=0;k<3;++k){
            // Operand fetch/store order preserves native aliasing and failed prefixes.
            const float a=pre?at(other,column*3+k):at(m.values,column*3+k);
            const float b=pre?at(m.values,row+k*3):at(other,row+k*3);
            const float product=a*b;sum=sum+product;output=sum;
        }
    }
    return set_matrix_values(m,work);
}
SpriteMatrix& rotate(SpriteMatrix& m,float angle,float cx,float cy,bool pre){
    number(angle);
    if(!m.rotation_work)m.rotation_work=std::make_shared<std::vector<float>>(9);
    const bool pivot=cx!=0.0F||cy!=0.0F;
    if(pivot){if(pre)pre_translate_matrix(m,cx,cy);else post_translate_matrix(m,-cx,-cy);}
    const float sine=degree_sine(angle),cosine=degree_cosine(angle);
    const float values[9]={cosine,sine,0,-sine,cosine,0,0,0,1};
    for(std::size_t i=0;i<9;++i)at(m.rotation_work,i)=values[i];
    if(pre)pre_concat_matrix(m,m.rotation_work);else post_concat_matrix(m,m.rotation_work);
    if(pivot){if(pre)pre_translate_matrix(m,-cx,-cy);else post_translate_matrix(m,cx,cy);}
    return m;
}
}
SpriteMatrix& reset_matrix(SpriteMatrix& m){
    auto& v=required(m.values.get());if(v.size()<9)throw std::out_of_range("sprite identity copy range");
    constexpr float identity[9]={1,0,0,0,1,0,0,0,1};std::copy_n(identity,9,v.begin());return m;
}
SpriteMatrix& set_matrix_values(SpriteMatrix& m,MatrixValues v){copy_values(v,m.values,required(v.get()).size());return m;}
SpriteMatrix& copy_matrix(SpriteMatrix& m,const SpriteMatrix* other){copy_values(required(other).values,m.values,required(m.values.get()).size());return m;}
SpriteMatrix& post_concat_matrix(SpriteMatrix& m,MatrixValues other){return concat(m,std::move(other),false);}
SpriteMatrix& pre_concat_matrix(SpriteMatrix& m,MatrixValues other){return concat(m,std::move(other),true);}
SpriteMatrix& set_concat_matrix(SpriteMatrix& m,const SpriteMatrix* left,const SpriteMatrix* right){
    // Native object overload requires both objects before touching destination.
    auto a=required(left).values,b=required(right).values;set_matrix_values(m,a);return post_concat_matrix(m,b);
}
SpriteMatrix& post_translate_matrix(SpriteMatrix& m,float x,float y){
    number(x);number(y);
    for(std::size_t column=0;column<3;++column){const auto i=column*3;const float z=at(m.values,i+2);
        const float dx=x*z,dy=y*z;at(m.values,i)=at(m.values,i)+dx;at(m.values,i+1)=at(m.values,i+1)+dy;}
    return m;
}
SpriteMatrix& pre_translate_matrix(SpriteMatrix& m,float x,float y){
    number(x);number(y);
    for(std::size_t row=0;row<3;++row){
        const float a=at(m.values,row)*x,b=at(m.values,row+3)*y;
        const float sum=a+b;at(m.values,row+6)=at(m.values,row+6)+sum;
    }
    return m;
}
SpriteMatrix& post_scale_matrix(SpriteMatrix& m,float x,float y,float cx,float cy){
    number(x);number(y);const bool pivot=cx!=0.0F||cy!=0.0F;if(pivot)post_translate_matrix(m,-cx,-cy);
    for(std::size_t column=0;column<3;++column){const auto i=column*3;at(m.values,i)=at(m.values,i)*x;at(m.values,i+1)=at(m.values,i+1)*y;}
    if(pivot)post_translate_matrix(m,cx,cy);
    return m;
}
SpriteMatrix& pre_scale_matrix(SpriteMatrix& m,float x,float y,float cx,float cy){
    number(x);number(y);const bool pivot=cx!=0.0F||cy!=0.0F;if(pivot)pre_translate_matrix(m,cx,cy);
    for(std::size_t i=0;i<6;++i)at(m.values,i)=at(m.values,i)*(i<3?x:y);
    if(pivot)pre_translate_matrix(m,-cx,-cy);
    return m;
}
SpriteMatrix& set_scale_matrix(SpriteMatrix& m,float x,float y,float cx,float cy){reset_matrix(m);return post_scale_matrix(m,x,y,cx,cy);}
SpriteMatrix& set_translate_matrix(SpriteMatrix& m,float x,float y){reset_matrix(m);return post_translate_matrix(m,x,y);}
SpriteMatrix& post_rotate_matrix(SpriteMatrix& m,float a,float x,float y){return rotate(m,a,x,y,false);}
SpriteMatrix& pre_rotate_matrix(SpriteMatrix& m,float a,float x,float y){return rotate(m,a,x,y,true);}
SpriteMatrix& set_rotate_matrix(SpriteMatrix& m,float a,float x,float y){reset_matrix(m);return post_rotate_matrix(m,a,x,y);}
void map_matrix_point(const SpriteMatrix& m,float& x,float& y){
    // The native body validates indices 0,3,6 before writing X, then 1,4,7.
    const auto& v=required(m.values.get());if(v.size()<=6)throw std::out_of_range("sprite matrix point range");
    const float old_x=x,old_y=y,a=old_x*v.at(0),b=old_y*v.at(3);
    const float sum=a+b;x=v.at(6)+sum;
    const float c=old_x*v.at(1),d=old_y*v.at(4);const float other=c+d;y=v.at(7)+other;
}
namespace {
double radians(float angle){const double scaled=static_cast<double>(angle)*3.141592653589793;return scaled/180.0;}
}
float degree_sine(float angle){return static_cast<float>(std::sin(radians(angle)));}
float degree_cosine(float angle){return static_cast<float>(std::cos(radians(angle)));}
float degree_tangent(float angle){return std::isfinite(angle)?static_cast<float>(std::tan(radians(angle))):0.0F;}
} // namespace saga::core
