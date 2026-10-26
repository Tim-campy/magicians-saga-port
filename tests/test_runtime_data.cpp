#include "platform/desktop/runtime_data.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga;
void check(bool v){if(!v){std::cerr<<"runtime data contract failure\n";std::exit(1);}}
template<class F>void rejects(F f){try{f();check(false);}catch(const std::invalid_argument&) {}}
void exercise(std::shared_ptr<const std::vector<std::int32_t>> primes){
    auto value=std::make_shared<int>(7),replacement=std::make_shared<int>(9);int calls=0;
    desktop::RuntimeCallContextHost host(primes,[&](core::AsyncValue v){check(v==value);++calls;return replacement;});
    core::TypedCallContextCopyHost copy_host(host);core::ThreadExecutionState thread;
    thread.context=std::make_shared<core::AsyncExecutionState>();auto original=thread.context;
    auto logical=std::make_shared<core::LogicalCallContext>();logical->correlation_manager=true;logical->data=host.make_store();
    logical->data->set(std::make_shared<std::u16string>(u"System.Diagnostics.Trace.CorrelationManagerSlot"),value);
    for(int i=0;i<20;++i)logical->data->set(std::make_shared<std::u16string>(1,static_cast<char16_t>(u'a'+i)),value);
    original->logical_call=logical;thread.belongs_to_outer_scope=true;
    core::OwnedAsyncExecutionHost execution(thread,copy_host);auto copied=execution.mutable_execution_context();
    check(copied!=original&&calls==1);
    auto context=std::static_pointer_cast<core::LogicalCallContext>(copied->logical_call);check(context->data->count()==21);
    auto entries=context->data->enumerate();int count=0;
    while(entries->next())if(entries->value()==replacement)++count;
    check(count==1&&logical->data->count()==21);
}
int main(int argc,char** argv){
    std::string bytes;std::int32_t candidate=3;
    for(int i=0;i<72;++i){while(!core::hashtable_is_prime(candidate))candidate+=2;
        for(unsigned j=0;j<4;++j)bytes.push_back(static_cast<char>((static_cast<std::uint32_t>(candidate)>>(8*j))&255U));candidate+=2;}
    auto primes=desktop::decode_hashtable_primes(bytes);check(primes->size()==72);exercise(primes);
    rejects([&]{desktop::decode_hashtable_primes(bytes.substr(1));});
    auto bad=bytes;bad[3]=static_cast<char>(0x80);rejects([&]{desktop::decode_hashtable_primes(bad);});
    bad=bytes;bad.replace(4,4,bytes.substr(0,4));rejects([&]{desktop::decode_hashtable_primes(bad);});
    bad=bytes;bad[0]=9;rejects([&]{desktop::decode_hashtable_primes(bad);});
    if(argc==2){exercise(desktop::load_hashtable_primes(argv[1]));std::cout<<"Local runtime table and context copy verified\n";}
}
