#include "collation_selection.hpp"
namespace saga::core {
// CompareInfo.get_UseManagedCollation 0x1dcd5a0: commit the cache after providers return.
bool use_managed_collation(CollationSelection& state,CollationHost& host){
    if(!state.initialized){
        const auto disabled=host.disable_managed_environment();
        const bool result=disabled!=std::optional<std::u16string>(u"yes")&&host.managed_tables_ready();
        state.use_managed=result;
        state.initialized=true;
    }
    return state.use_managed;
}
// CompareInfo.IsPrefix helper 0x1dcabb4; length guard belongs only to fallback.
bool cultural_prefix(CollationSelection& state,CollationHost& host,std::u16string_view source,
                     std::u16string_view prefix,std::int32_t options){
    if(use_managed_collation(state,host))return host.managed_prefix(source,prefix,options);
    if(source.size()<prefix.size())return false;
    return host.compare_slices(source.substr(0,prefix.size()),prefix,options)==0;
}
}
