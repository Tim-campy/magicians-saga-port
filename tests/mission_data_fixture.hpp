#pragma once
#include "jar_fixture.hpp"
#include "platform/desktop/gameplay_catalog.hpp"

namespace saga::test {
inline std::string mission_parameters(){std::string out="13";for(int i=0;i<13;++i)out+="\t2\t100\t1000";return out;}
inline std::vector<std::pair<std::string,Bytes>> mission_tables(){
    const auto p=mission_parameters();auto bytes=[](std::string value){return Bytes(value.begin(),value.end());};
    std::string shop;for(int i=0;i<8;++i)shop+=std::to_string(i)+"\tUpgrade\t0\t1\t2\tText\t0\n";
    return {
        {"English.lproj/Chara.txt",bytes("2\tHero\t0\tchara\t8\tchara\t1\t0\t0\t99\t0\t100\t"+p+"\t0\t0\t0\t-1\t0\t2")},
        {"English.lproj/Equipment.txt",bytes("1\tWeapon\t0\t2\t0\t0\tweapon\t12\t3\t0\t0\t0\t0\t10\t8\t80\t1\t20\t3\t0\t0\t"+p+"\tText\t0")},
        {"English.lproj/Cloth.txt",bytes("0\tCloth\t0\tcloth\t11\t0\t0\t0\t0\t"+p+"\tText\t0")},
        {"English.lproj/Hat.txt",bytes("0\tHat\t0\that\t9\t0\t0\t0\t0\t"+p+"\tText\t2")},
        {"English.lproj/MagicShop.txt",bytes(shop)},
        {"English.lproj/Enemy.txt",bytes("4\tEnemy\t0\t2\t10\t100\t0\t0\t0\tenemy-book\t7\t0\t0\t0\t"+p+"\t0")},
        {"English.lproj/Bullet.txt",bytes("3\t0\t0\tbullet\t5\t6\teffect\t7\t0")}
    };
}
} // namespace saga::test
