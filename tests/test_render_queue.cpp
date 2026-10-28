#include "core/gameplay/render_queue.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
RenderCommand command(std::int32_t depth=0,std::int32_t id=0){RenderCommand r;r.values[render_depth]=depth;r.values[render_texture]=id;return r;}
int main(){
    {auto q=make_render_queue();RenderCommand c;for(std::size_t i=0;i<19;++i)c.values[i]=static_cast<std::int32_t>(i+100);
     auto empty=q.buffers->at(0);insert_render_command(q,c);auto grown=q.buffers->at(0);check(grown!=empty&&grown->size()==128&&q.counts->at(0)==1);
     check(*grown->at(0)==EventIntegers(c.values.begin(),c.values.end()));auto row=grown->at(0);clear_render_queue(q);check(q.counts->at(0)==0&&grown->at(0)==row);
     c.values[0]=-1;insert_render_command(q,c);check(q.buffers->at(0)==grown&&grown->at(0)==row&&row->at(0)==-1);
     for(int i=1;i<129;++i)insert_render_command(q,command(i));check(q.buffers->at(0)->size()==256&&q.buffers->at(0)->at(0)==row&&grown->size()==128);}
    {auto q=make_render_queue();insert_render_command(q,command(2,20));insert_render_command(q,command(1,10));insert_render_command(q,command(1,11));
     auto a=q.buffers->at(0)->at(1),b=q.buffers->at(0)->at(2);sort_render_queue(q);
     check(q.buffers->at(0)->at(0)==b&&q.buffers->at(0)->at(1)==a&&q.buffers->at(0)->at(2)->at(render_texture)==20);}
    {auto q=make_render_queue();for(int i=0;i<5;++i)insert_render_command(q,command(7,i));auto rows=q.buffers->at(0);auto snapshot=*rows;
     sort_render_queue(q);check(*rows==snapshot&&render_pivot(rows,0,4)==-1);}
    {auto q=make_render_queue();for(auto value:{5,-7,3,1,-7,0,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()})insert_render_command(q,command(value));
     auto second=command(-2,99);second.type=1;insert_render_command(q,second);sort_render_queue(q);
     auto rows=q.buffers->at(0);for(int i=1;i<q.counts->at(0);++i)check(rows->at(static_cast<std::size_t>(i-1))->at(2)<=rows->at(static_cast<std::size_t>(i))->at(2));
     check(q.counts->at(1)==1&&q.buffers->at(1)->at(0)->at(0)==99);}
    {auto q=make_render_queue();insert_render_command(q,command(3));auto short_row=std::make_shared<EventIntegers>(2,0);q.buffers->at(0)->at(1)=short_row;
     auto c=command(1,42);c.values[1]=43;fails([&]{insert_render_command(q,c);});check(q.counts->at(0)==1&&*short_row==EventIntegers({42,43}));}
    {auto q=make_render_queue();q.counts->at(0)=300;auto old=q.buffers->at(0);fails([&]{insert_render_command(q,command());});
     check(q.buffers->at(0)!=old&&q.buffers->at(0)->size()==128&&q.counts->at(0)==300);}
    {auto q=make_render_queue();q.counts->at(0)=-1;fails([&]{insert_render_command(q,command());});check(q.buffers->at(0)->empty());
     auto c=command();c.type=-1;fails([&]{insert_render_command(q,c);});q.counts->at(0)=0;q.buffers.reset();sort_render_queue(q);
     q.counts->at(0)=1;fails([&]{sort_render_queue(q);});q.counts.reset();fails([&]{clear_render_queue(q);});}
    check(render_pivot(nullptr,2,1)==-1);quicksort_render_rows(nullptr,0,0);check(partition_render_rows(nullptr,2,1,0)==2);
    // All three game-specific partition helpers are exercised by varied duplicate-depth inputs.
    for(int seed=0;seed<24;++seed){auto q=make_render_queue();for(int i=0;i<37;++i)insert_render_command(q,command((i*17+seed*13)%11-5,i));
     sort_render_queue(q);auto& rows=*q.buffers->at(0);for(int i=1;i<37;++i)check(rows[static_cast<std::size_t>(i-1)]->at(2)<=rows[static_cast<std::size_t>(i)]->at(2));}
}
