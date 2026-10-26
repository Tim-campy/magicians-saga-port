#include "core/gameplay/collision_check.hpp"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Fixture {
    ComponentSlots a{35},b{35};ContactPosition pa,pb;ProjectileCollision ca,cb;
    Fixture(){ca.radius=2;cb.radius=3;a.add(0,&pa).add(32,&ca);b.add(0,&pb).add(32,&cb);}
    bool hit(){return check_collision(&a,&b);}
};
int main(){
    {Fixture f;f.pb.x=3;f.pb.y=4;check(f.hit());f.pb.y=std::nextafter(4.0F,5.0F);check(!f.hit());}
    {Fixture f;f.pb.z=6;check(!f.hit());f.cb.offset_z=-1;check(f.hit());f.ca.offset_x=5;check(!f.hit());}
    {Fixture f;f.ca.radius=-2;f.cb.radius=-3;f.pb.x=5;check(f.hit());}
    {Fixture f;f.ca.radius=46341;f.cb.radius=0;check(!f.hit());}
    {Fixture f;f.ca.radius=std::numeric_limits<std::int32_t>::max();f.cb.radius=1;check(f.hit());f.pb.z=1;check(!f.hit());}
    {Fixture f;f.pa.x=std::numeric_limits<float>::quiet_NaN();check(!f.hit());}
    {Fixture f;f.pb.z=std::numeric_limits<float>::infinity();check(!f.hit());}
    {Fixture f;f.ca.radius=f.cb.radius=0;f.pa.x=f.pb.x=16777216.0F;f.ca.offset_x=1;check(f.hit());} // Binary32 center rounding.
    {Fixture f;f.a.remove(32);check(!check_collision(&f.a,nullptr));}
    {Fixture f;f.b.remove(32);f.a.remove(0);check(!f.hit());}
    {Fixture f;bool failed=false;try{check_collision(nullptr,&f.b);}catch(const std::invalid_argument&){failed=true;}check(failed);}
    {Fixture f;bool failed=false;try{check_collision(&f.a,nullptr);}catch(const std::invalid_argument&){failed=true;}check(failed);}
    {Fixture f;f.a.remove(0);bool failed=false;try{f.hit();}catch(const std::invalid_argument&){failed=true;}check(failed);}
    // Small integer centers have exact binary32 sums/squares; compare against an independent int64 oracle.
    for(int i=0;i<2000;++i){Fixture f;
        const int x=i%31-15,y=i%23-11,z=i%17-8,ox=i%7-3,oy=i%5-2,oz=i%3-1;
        f.pb.x=static_cast<float>(x);f.pb.y=static_cast<float>(y);f.pb.z=static_cast<float>(z);
        f.cb.offset_x=ox;f.cb.offset_y=oy;f.cb.offset_z=oz;f.ca.radius=i%13;f.cb.radius=i%11;
        const std::int64_t dx=x+ox,dy=y+oy,dz=z+oz,r=f.ca.radius+f.cb.radius;
        check(f.hit()==(dx*dx+dy*dy+dz*dz<=r*r));check(f.hit()==check_collision(&f.b,&f.a));}
}
