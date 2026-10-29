#include "seb_sprite.hpp"
#include <limits>

namespace saga::core {
SebSpriteState clone_seb_sprites(const SebSpriteState& source){
    SebSpriteState result;
    result.groups=source.groups;result.frames=source.frames;result.packed=source.packed;result.extended=source.extended;
    return result;
}
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing SEB runtime array");return *p;}
template<class T>T& at(const std::shared_ptr<std::vector<T>>& array,std::int32_t i){return required(array.get()).at(static_cast<std::size_t>(i));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t signed_byte(std::uint8_t v){return v<128?v:static_cast<std::int32_t>(v)-256;}
std::int16_t short_value(std::int32_t v){const auto n=static_cast<std::uint32_t>(v)&65535U;const auto signed_value=n<32768?static_cast<std::int32_t>(n):static_cast<std::int32_t>(n)-65536;return static_cast<std::int16_t>(signed_value);}
std::int32_t length(std::size_t n){if(n>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("SEB array exceeds int32");return static_cast<std::int32_t>(n);}
std::int32_t divide(std::int32_t a,std::int32_t b){ // ARM SDIV, including zero/overflow.
    if(b==0)return 0;
    if(a==std::numeric_limits<std::int32_t>::min()&&b==-1)return a;
    return a/b;
}
std::int32_t blend(std::int32_t a,std::int32_t b,std::int32_t elapsed,std::int32_t span){
    const auto slope=divide(word(static_cast<std::uint32_t>(sub(b,a))<<12U),span);
    const auto product=word(static_cast<std::uint32_t>(slope)*static_cast<std::uint32_t>(elapsed));
    return add(a,product/4096); // Negative-product correction precedes arithmetic shift in native code.
}
std::int32_t reflect(std::int32_t size,std::int32_t position){
    const auto rounded=short_value(add(size,static_cast<std::int32_t>((static_cast<std::uint32_t>(size)>>15U)&1U)));
    return word(0U-((static_cast<std::uint32_t>(static_cast<std::int32_t>(rounded))>>1U)<<1U)-static_cast<std::uint32_t>(position));
}
SpriteRows matrix(std::int32_t rows){
    if(rows<0)throw std::length_error("negative SEB matrix size");
    auto result=std::make_shared<std::vector<SpriteArray>>(static_cast<std::size_t>(rows));
    for(auto& row:*result)row=std::make_shared<SpriteValues>(12,0);
    return result;
}
SpriteRows instance_rows(SebSpriteState& s){
    if(!s.work)s.work=std::make_shared<SebInstanceWork>();
    if(!s.work->sprites)s.work->sprites=matrix(length(required(s.groups.get()).size()));
    return s.work->sprites;
}
SpriteRows local_rows(SebStaticWork& w,std::int32_t layers){
    auto& old=required(w.local.get());
    if(layers>=length(old.size())){
        const auto count=add(layers,5);if(count<0)throw std::length_error("negative SEB work size");
        w.local=std::make_shared<std::vector<SpriteRows>>(static_cast<std::size_t>(count));
        for(std::int32_t i=0;i<count;++i)at(w.local,i)=matrix(i);
    }
    return at(w.local,layers);
}
SpriteArray cached(SebSpriteState& s,std::int32_t frame,std::int32_t layer,bool& hit){
    auto columns=s.cache;auto& values=required(columns.get());const auto count=length(values.size());
    std::int32_t selected=count-1;
    for(std::int32_t i=0;i<count;++i){
        const auto key=at(at(columns,i),layer);const auto previous=at(key,0);
        if(previous==-1)break;
        if(previous==frame){selected=i;hit=true;break;}
    }
    auto output=at(at(columns,selected),layer);
    for(auto i=selected;i>0;--i)at(at(s.cache,i),layer)=at(at(s.cache,i-1),layer);
    at(at(s.cache,0),layer)=output;
    return at(at(s.cache,0),layer);
}
SpriteArray evaluate(SebSpriteState& s,SebStaticWork& work,SpriteRows fallback,std::int32_t raw,std::int32_t layer){
    const auto frame=raw%10000;
    SpriteArray output;
    if(s.cache){bool hit=false;output=cached(s,frame,layer,hit);if(hit)return output;}
    else output=at(fallback,layer);
    const auto groups=length(required(s.groups.get()).size());auto first=at(s.groups,layer);
    const auto total=length(required(s.packed.get()).size())/8;
    const auto next=add(layer,1);auto last=sub(next<groups?at(s.groups,next):total,1);
    if(last<first)return nullptr;
    if(frame<at(s.frames,first)||frame>at(s.frames,last))return nullptr;
    if(frame>=2&&add(sub(last,first),1)>=3){
        while(true){const auto mid=add(first,sub(last,first)/2);const auto key=at(s.frames,mid);
            if(frame==key)break;
            if(frame>key)first=mid;else last=mid;
            if(add(sub(last,first),1)<=2)break;
        }
    }
    std::int32_t previous=0,following=0,elapsed=0,span=0;
    for(auto i=first;i<=last;i=add(i,1)){
        const auto key=at(s.frames,i);
        if(frame==key){
            convert_seb_sprite(s,i,output,raw>9999?1:0);
            if(s.flip)at(output,8)=at(output,8)==0?1:0;
            return output;
        }
        if(frame<key){following=i;previous=sub(i,1);const auto before=at(s.frames,previous);span=sub(key,before);elapsed=sub(frame,before);break;}
    }
    auto left=work.left;convert_seb_sprite(s,previous,left,raw>9999?1:0);
    auto right=work.right;convert_seb_sprite(s,following,right,raw>9999?1:0);
    at(output,0)=frame;at(output,1)=at(left,1);
    for(std::int32_t i=2;i<=5;++i)at(output,i)=at(left,1)<0?blend(at(left,i),at(right,i),elapsed,span):at(left,i);
    for(std::int32_t i=6;i<=7;++i)at(output,i)=blend(at(left,i),at(right,i),elapsed,span);
    at(output,8)=at(left,8);at(output,9)=at(left,9);
    // Both GetSprite bodies overwrite even the negative-TexId crop interpolation.
    for(std::int32_t i=2;i<=5;++i)at(output,i)=at(left,i);
    at(output,10)=at(left,10);at(output,11)=at(left,11);
    if(s.flip)at(output,8)=at(output,8)==0?1:0;
    return output;
}
}
SebStaticWork::SebStaticWork():local(std::make_shared<std::vector<SpriteRows>>(1)),lists(std::make_shared<std::vector<SpriteRows>>()),
    left(std::make_shared<SpriteValues>(12,0)),right(std::make_shared<SpriteValues>(12,0)){
    local->at(0)=std::make_shared<std::vector<SpriteArray>>();
}
SebSpriteState prepare_seb_sprites(const assets::Seb& source){
    if(source.mode>1)throw std::invalid_argument("unsupported SEB preparation mode");
    SebSpriteState s;s.groups=std::make_shared<std::vector<std::int32_t>>();s.frames=std::make_shared<std::vector<std::int16_t>>();
    s.packed=std::make_shared<std::vector<std::uint8_t>>();s.extended=std::make_shared<std::vector<std::int16_t>>();
    SpriteValues v(12,0);std::size_t extended_count=0;
    for(const auto& group:source.groups){
        s.groups->push_back(length(s.frames->size()));
        for(std::size_t n=0;n<group.record_count;++n){
            const auto& r=source.records.at(static_cast<std::size_t>(group.first_record)+n);
            if(source.mode==0){
                for(std::size_t i=0;i<10;++i)v[i]=r.values[i];v[10]=0;v[11]=-1;
                if(v[1]<0){const auto flag=static_cast<std::uint32_t>(v[8])&0xff00U;
                    if(flag==0x2000U)v[10]=1;else if(flag==0x4000U)v[10]=2;else if(flag==0x8000U)v[10]=3;
                    const auto alpha=v[10]!=0?0x80000000U:0xff000000U;
                    v[11]=word(alpha|(static_cast<std::uint32_t>(v[8])&65535U)|((static_cast<std::uint32_t>(v[9])&255U)<<16U));
                }
            }else{
                v[0]=r.values[0];v[1]=r.values[1];v[6]=r.values[2];v[7]=r.values[3];v[10]=r.control.value();v[8]=0;v[9]=0;
                if(r.extra_word)v[11]=word(*r.extra_word);
                if(r.pair){v[2]=r.pair->at(0);v[3]=r.pair->at(1);}
                v[4]=r.tail_pair.value().at(0);v[5]=r.tail_pair.value().at(1);
                if(v[1]>=0){const auto flags=r.flags.value();v[8]=flags&1;v[9]=(flags>>1U)&1;}
            }
            if((static_cast<std::uint32_t>(v[11])>>24U)==255U&&v[10]==1)v[10]=0;
            std::array<std::uint8_t,8> bytes{static_cast<std::uint8_t>(v[1]),static_cast<std::uint8_t>(v[6]),static_cast<std::uint8_t>(v[7]),
                static_cast<std::uint8_t>(v[2]),static_cast<std::uint8_t>(v[3]),static_cast<std::uint8_t>(v[4]),static_cast<std::uint8_t>(v[5]),
                static_cast<std::uint8_t>((static_cast<std::uint32_t>(v[8])<<6U)|(static_cast<std::uint32_t>(v[9])<<5U)|static_cast<std::uint32_t>(v[10]))};
            bool extended=v[10]!=0||v[1]==-8||v[1]==-4||v[1]==-3||v[1]==-2;
            extended=extended||v[1]!=signed_byte(bytes[0])||v[6]!=signed_byte(bytes[1])||v[7]!=signed_byte(bytes[2]);
            for(std::size_t i=2;i<=5;++i)extended=extended||v[i]!=bytes[i+1];
            if(extended){bytes[0]=128;bytes[1]=static_cast<std::uint8_t>(extended_count++);
                const auto color=static_cast<std::uint32_t>(v[11]);for(unsigned i=0;i<4;++i)bytes[i+2]=static_cast<std::uint8_t>(color>>(24U-i*8U));
                for(const auto i:{1,6,7,2,3,4,5})s.extended->push_back(short_value(v[static_cast<std::size_t>(i)]));
            }
            s.frames->push_back(short_value(v[0]));s.packed->insert(s.packed->end(),bytes.begin(),bytes.end());
        }
    }
    return s;
}
void convert_seb_sprite(const SebSpriteState& s,std::int32_t record,SpriteArray output,std::int32_t reverse){
    at(output,0)=at(s.frames,record);const auto offset=word(static_cast<std::uint32_t>(record)<<3U);
    const auto first=signed_byte(at(s.packed,offset));
    if(first==-128){
        std::uint32_t color=0;for(std::int32_t i=2;i<6;++i)color=(color<<8U)|at(s.packed,add(offset,i));at(output,11)=word(color);
        const auto extra=static_cast<std::int32_t>(at(s.packed,add(offset,1)))*7;
        std::int32_t i=0;for(const auto field:{1,6,7,2,3,4,5})at(output,field)=at(s.extended,add(extra,i++));
    }else{
        at(output,1)=first;at(output,6)=signed_byte(at(s.packed,add(offset,1)));at(output,7)=signed_byte(at(s.packed,add(offset,2)));
        for(std::int32_t i=2;i<6;++i)at(output,i)=at(s.packed,add(offset,i+1));
        // Compact records never reset Color: the output array retains its previous cell 11.
    }
    const auto flags=at(s.packed,add(offset,7));at(output,10)=flags&15;at(output,8)=(flags>>6U)&1;at(output,9)=(flags>>5U)&1;
    if((static_cast<std::uint32_t>(reverse)&1U)!=((s.reverse>>1U)&1U)){
        at(output,8)^=1;
        if(at(output,1)==-8){for(const auto i:{6,2,4})at(output,i)=sub(0,at(output,i));}
        else at(output,6)=reflect(at(output,4),at(output,6));
    }
    if((s.reverse&1U)!=0){
        at(output,9)^=1;
        if(at(output,1)==-8){for(const auto i:{7,3,5})at(output,i)=sub(0,at(output,i));}
        else at(output,7)=reflect(at(output,5),at(output,7));
    }
}
SpriteArray seb_sprite(SebSpriteState& s,SebStaticWork& w,std::int32_t frame,std::int32_t layer){
    auto rows=instance_rows(s);
    return evaluate(s,w,rows,frame,layer);
}
SpriteArray seb_sprite_local(SebSpriteState& s,SebStaticWork& w,std::int32_t frame,std::int32_t layer){
    const auto layers=length(required(s.groups.get()).size());auto rows=local_rows(w,layers);
    return evaluate(s,w,rows,frame,layer);
}
SpriteRows seb_sprites_local(SebSpriteState& s,SebStaticWork& w,std::int32_t frame){
    const auto layers=length(required(s.groups.get()).size());
    if(!w.lists||layers>=length(w.lists->size())){
        w.lists=std::make_shared<std::vector<SpriteRows>>(static_cast<std::size_t>(add(layers,1)));
        for(std::int32_t i=0;i<=layers;++i)at(w.lists,i)=std::make_shared<std::vector<SpriteArray>>(static_cast<std::size_t>(i));
    }
    auto output=at(w.lists,layers);for(std::int32_t i=0;i<layers;++i)at(output,i)=seb_sprite_local(s,w,frame,i);
    return output;
}
void set_seb_cache_size(SebSpriteState& s,std::int32_t size){
    if(size<0)throw std::length_error("negative SEB cache size");
    s.cache=std::make_shared<std::vector<SpriteRows>>(static_cast<std::size_t>(size));
    for(std::int32_t i=0;i<size;++i){auto rows=matrix(length(required(s.groups.get()).size()));at(s.cache,i)=rows;
        for(auto& sprite:*rows)at(sprite,0)=-1;
    }
}
void reverse_seb(SebSpriteState& s,std::int32_t horizontal,std::int32_t vertical){
    if(s.cache)for(auto& column:*s.cache)for(auto& sprite:required(column.get()))at(sprite,0)=-1;
    s.reverse=static_cast<std::uint8_t>(((static_cast<std::uint32_t>(horizontal)&1U)<<1U)|(static_cast<std::uint32_t>(vertical)&1U));
}
} // namespace saga::core
