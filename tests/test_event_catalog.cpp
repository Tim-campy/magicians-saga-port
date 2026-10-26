#include "core/gameplay/event_catalog.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v,const char* message) { if (!v) {std::cerr<<message;std::exit(1);} }
template<class F> void fails(F f) {try {f();} catch(const std::exception&) {return;} check(false,"expected failure");}
int main() {
    auto load=[](std::string_view s,bool normalize=false) {return load_event_catalog(s,normalize,parse_event_decimal);};
    check(load("").size()==1 && !load("")[0],"empty input has one null slot");
    auto data=load("\xef\xbb\xbf" "0\tfirst\t0\t-1\t1\t2\t10\t3\r\n\n2\tother\t0\t0\t0\n2\tlast\t1\t1\t2\t4\t0\t0\tignored");
    check(data.size()==3 && !data[1] && data[2]->name=="last","id indexing, gap and last duplicate");
    check(!data[0]->terms && data[2]->effects && data[2]->effects->empty(),"null versus empty script");
    check(data[2]->execution_count==0 && data[2]->timing==1,"counter and timing");
    check(data[0]->effects->at(0)->at(1)==3,"trailing CR accepted by integer parser");
    int fallback=0;
    check(identify_event_script(data,data[2]->effects,[&]{++fallback;})==2,"script identity uses id");
    auto clone=std::make_shared<std::vector<EventInstruction>>(*data[2]->effects);
    check(identify_event_script(data,clone,[&]{++fallback;})==-1 && fallback==1,"clone does not match");
    check(resolve_event_script(data,-1000)==data[0]->effects && !resolve_event_script(data,1000),"delay remainder and upper guard");
    fails([&]{resolve_event_script(data,-1);});fails([&]{resolve_event_script(data,1);});
    fails([&]{load("-1\tx\t0\t0\t0");});
    fails([&]{load("2147483647\tx\t0\t0\t0");});
    fails([&]{load("0\tx\t0\t1\t-1\t0");});
    fails([&]{load("0\tx\t0\t1\t2\t1");});
    fails([&]{load("\r\n");});check(load("\r\n",true).size()==1,"version-gated CRLF normalization");
    fails([&]{load("\r",true);});fails([&]{load(" ");});
    check(parse_event_decimal(" -2147483648\r")==std::numeric_limits<std::int32_t>::min(),"minimum int32");
    fails([&]{parse_event_decimal("2147483648");});fails([&]{parse_event_decimal("");});
    fails([&]{parse_event_decimal("1.0");});
    auto nulls=load("0\tx\t0\t0\t-1");
    check(identify_event_script(nulls,nullptr,[]{throw std::logic_error("unexpected fallback");})==0,"null script identity");
}
