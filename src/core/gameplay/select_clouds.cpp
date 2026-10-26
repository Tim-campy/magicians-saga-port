#include "select_clouds.hpp"
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
std::int32_t wrap(std::int64_t v) {
    const auto bits=static_cast<std::uint32_t>(v);
    return bits<=0x7fffffffU ? static_cast<std::int32_t>(bits)
        : static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
template<class T> T& required(const std::shared_ptr<T>& value) {
    if (!value) throw std::invalid_argument("null SelectForm cloud data");
    return *value;
}
}
// SelectForm.Update, ELF 0x11ebe64..0x11ec0bc. Counter already advanced by caller.
void update_select_clouds(const std::int32_t& counter, SelectCloudHost& host) {
    const auto count=required(host.clouds()).size();
    if (count>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("cloud count exceeds managed int32");
    for (auto i=count;i>0;--i) {
        const auto index=i-1;
        const auto cloud=required(host.clouds()).at(index);
        auto& values=required(cloud);
        values.at(6)=wrap(static_cast<std::int64_t>(values.at(6))+1);
        if (values.at(6)>=values.at(7)) {
            if (counter%3==0) values.at(0)=wrap(static_cast<std::int64_t>(values.at(0))+1);
            values.at(6)=0;
        }
        const auto x=values.at(0);
        if (x<=host.right_edge()) continue;
        auto& old_list=required(host.clouds());
        if(index>=old_list.size()) throw std::out_of_range("cloud removal index");
        old_list.erase(old_list.begin()+static_cast<std::ptrdiff_t>(index));
        const auto resource=host.cloud_resource();
        const auto height=host.game_height();
        const auto offset=host.low_aspect() ? -285 : -280;
        const auto y_random=host.random(30);
        const auto variant=host.random(4);
        const auto destination=host.clouds();
        const auto y=wrap(static_cast<std::int64_t>(height)+offset-y_random);
        auto replacement=std::make_shared<EventIntegers>(EventIntegers{-30,y,resource,0,1,variant,0,20});
        required(destination).push_back(std::move(replacement));
    }
}
}
