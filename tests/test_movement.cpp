#include "core/gameplay/movement.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Fixture {
    ComponentSlots e{35},parent{35};ContactPosition p,pp;ProjectileSpeed v;
    Fixture(){p.x=1;p.y=2;p.z=3;v.x=4;v.y=-5;v.z=6;
        pp.x=100;pp.y=200;pp.z=300;p.offset_x=7;p.offset_y=8;p.offset_z=9;
        e.add(0,&p).add(1,&v);parent.add(0,&pp);}
};
int main(){
    {Fixture f;move_entity(f.e);check(f.p.x==5&&f.p.y==-3&&f.p.z==9&&f.v.x==4);}
    {Fixture f;f.p.parent=&f.parent;move_entity(f.e);check(f.p.x==107&&f.p.y==208&&f.p.z==309);}
    {Fixture f;f.p.parent=&f.parent;f.parent.remove(0);move_entity(f.e);check(f.p.x==5&&f.p.y==-3&&f.p.z==9);}
    {Fixture f;f.p.parent=&f.e;move_entity(f.e);check(f.p.x==12&&f.p.y==5&&f.p.z==18);}
    {Fixture f;f.p.parent=&f.parent;Component wrong;f.parent.change(0,&wrong);bool failed=false;
        try{move_entity(f.e);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.p.x==5&&f.p.y==-3&&f.p.z==9);}
    {Fixture f;f.e.remove(1);bool failed=false;try{move_entity(f.e);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.p.x==1&&f.p.y==2&&f.p.z==3);}
    {Fixture f;f.p.x=16777216.0F;f.v.x=1;move_entity(f.e);check(f.p.x==16777216.0F);}
    {Fixture f;ProjectileSpeed parent_speed;parent_speed.x=10;f.parent.add(1,&parent_speed);f.p.parent=&f.parent;
        move_entity(f.e);move_entity(f.parent);check(f.p.x==107&&f.pp.x==110);
        move_entity(f.parent);move_entity(f.e);check(f.p.x==127);}
}
