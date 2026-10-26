#include "subset_cursors.hpp"

namespace saga::core {
namespace {
class Contacts final:public CollisionCursor {
public:
    explicit Contacts(EntitySubset& group):cursor_(group.cursor()){}
    std::optional<ComponentSlots*> next()override{return cursor_.next();}
private:
    EntitySubset::Cursor cursor_;
};
class Garbage final:public GarbageCursor {
public:
    Garbage(EntitySubset& group,std::function<QuestMemberId(const ComponentSlots&)> identity)
        :cursor_(group.cursor()),identity_(std::move(identity)){}
    std::optional<GarbageEntity> next()override{
        const auto entity=cursor_.next();if(!entity)return std::nullopt;
        return GarbageEntity{identity_(**entity),*entity};
    }
private:
    EntitySubset::Cursor cursor_;
    std::function<QuestMemberId(const ComponentSlots&)> identity_;
};
}
std::unique_ptr<CollisionCursor> open_collision_subset(EntitySubset& group){return std::make_unique<Contacts>(group);}
std::unique_ptr<GarbageCursor> open_garbage_subset(EntitySubset& group,std::function<QuestMemberId(const ComponentSlots&)> identity){return std::make_unique<Garbage>(group,std::move(identity));}
}
