#include "apply_task.hpp"
#include <limits>
#include <stdexcept>
namespace saga::core {
std::shared_ptr<ApplyCatalogItem> resolve_apply_name_item(const std::shared_ptr<ApplyCatalog>& catalog,
    const std::u16string& type,const std::u16string& text,std::int32_t value) {
    if(!catalog)throw std::invalid_argument("missing billing catalog");
    if(catalog->size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("billing catalog exceeds int32");
    std::shared_ptr<ApplyCatalogItem> exact,numeric,fallback;
    for(std::size_t i=0;i<catalog->size();++i) {
        const auto item=catalog->at(i);
        if(!item)continue;
        if(!item->provision_type)throw std::invalid_argument("null provision type");
        if(*item->provision_type!=type)continue;
        if(!item->provision_text)throw std::invalid_argument("null provision value text");
        if(*item->provision_text==text)exact=item;
        else if(item->provision_value>=2 && value>=2)numeric=item;
        else fallback=item;
    }
    return exact?exact:(numeric?numeric:fallback);
}
}
