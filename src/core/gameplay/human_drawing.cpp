#include "human_drawing.hpp"
#include <array>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* value){if(!value)throw std::invalid_argument("missing human drawing data");return *value;}
template<class T>T& required(const std::shared_ptr<T>& value){return required(value.get());}
template<class T>T& required(std::optional<T>& value){if(!value)throw std::invalid_argument("missing human drawing record");return *value;}
std::int32_t character_value(ScaledHumanDrawingHost& host,std::int32_t id,std::int32_t CharacterProgressionRecord::* field){
    // Retain the catalog owner until this individual read has completed.
    const auto rows=host.human_characters();return required(required(rows).at(static_cast<std::size_t>(id))).*field;
}
std::int32_t product(std::int32_t left,std::int32_t right){
    const auto bits=static_cast<std::uint32_t>(left)*static_cast<std::uint32_t>(right);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
}
std::int32_t remainder(std::int32_t frame,std::int32_t maximum){
    if(maximum==0)return frame;if(maximum==-1)return 0;return frame%maximum;
}
}
EventInstruction resolve_human_images(std::int32_t id,ScaledHumanDrawingHost& host){
    auto images=std::make_shared<EventIntegers>(5,0);
    images->at(0)=200;images->at(1)=character_value(host,id,&CharacterProgressionRecord::image);
    images->at(2)=character_value(host,id,&CharacterProgressionRecord::image);images->at(3)=-1;
    // Data.equipmentData is captured before the fresh Chara weapon-ID lookup.
    const auto weapons=host.human_weapons();const auto weapon=character_value(host,id,&CharacterProgressionRecord::weapon_id);
    images->at(4)=required(required(weapons).at(static_cast<std::size_t>(weapon))).image;
    if((static_cast<std::uint32_t>(character_value(host,id,&CharacterProgressionRecord::flags))&2U)!=0){
        images=std::make_shared<EventIntegers>(5,0);images->at(0)=200;
        images->at(1)=host.hero_cloth_image();images->at(2)=host.hero_cloth_image();
        images->at(3)=host.hero_hat_image();images->at(4)=host.hero_weapon_image();
    }
    return images;
}
void draw_human_images(const HumanDrawingRequest& request,EventInstruction images,RenderGraphics* graphics,ScaledHumanDrawingHost& host,SebStaticWork& work){
    const auto scale=static_cast<float>(request.scale)/100.0F;
    auto& matrix=host.human_scale_matrix();matrix.set_values({scale,0,0,0,scale,0,0,0,1});
    auto& target=required(graphics);target.push_matrix(matrix);
    // Same initializer field as World.CreateHuman (GOT0x272fdc8).
    constexpr std::array<std::int32_t,5> resources{1,1,1,21,22};
    auto& source=required(host.human_character_resource());
    const auto clips=required(source.animations).clips;
    auto& clip=required(required(clips).at(static_cast<std::size_t>(request.clip)));
    const auto frame=remainder(request.frame,clip.max_frame);
    const auto rows=seb_sprites(required(clip.sprites),work,frame);
    const auto& sprites=required(rows);
    if(sprites.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("human sprite count exceeds int32");
    const auto x=static_cast<float>(request.x),y=static_cast<float>(request.y);
    for(std::size_t index=0;index<sprites.size();++index){
        if(index>=3&&index<=4&&(static_cast<std::uint32_t>(request.mask)&1U)!=0)continue;
        if(index==0&&(static_cast<std::uint32_t>(request.mask)&2U)!=0)continue;
        const auto resource=resources.at(index);if(resource<0)continue;
        const auto texture=required(images).at(index);if(texture==-1||texture==-2)continue;
        const auto row=sprites.at(index);auto& sprite=required(row);
        if(sprite.at(1)==-1)continue;
        auto& drawing=required(host.human_resource(resource));
        auto* image=required(drawing.images).at(static_cast<std::size_t>(texture));
        static_cast<void>(sprite.at(9));
        const auto horizontal=sprite.at(8)!=0,vertical=sprite.at(9)!=0;
        const auto reverse=static_cast<std::int32_t>(horizontal)+2*static_cast<std::int32_t>(vertical);
        if(reverse)target.reverse=reverse;
        RenderImageDraw draw;draw.image=image;
        draw.u=sprite.at(2);draw.v=sprite.at(3);draw.source_width=sprite.at(4);draw.source_height=sprite.at(5);
        const auto offset_x=static_cast<float>(product(sprite.at(6),request.scale))/100.0F;
        const auto offset_y=static_cast<float>(product(sprite.at(7),request.scale))/100.0F;
        draw.x=x+offset_x;draw.y=y+offset_y;target.draw_image(draw);
        if(reverse)target.reverse=0;
    }
    target.pop_matrix();
}
void draw_scaled_human(const HumanDrawingRequest& request,RenderGraphics* graphics,ScaledHumanDrawingHost& host,SebStaticWork& work){
    auto images=resolve_human_images(request.character,host);
    draw_human_images(request,std::move(images),graphics,host,work);
}
} // namespace saga::core
