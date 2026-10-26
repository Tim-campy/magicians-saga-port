#include "apply_task.hpp"
#include "presentation_text.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
namespace saga::core {
namespace {
bool number_space(char16_t value) {return value==u' ' || (value>=u'\t' && value<=u'\r');}
std::u16string wrap_secure_text(std::optional<std::u16string> value) {
    // SecureString(string)->Set: null input faults before the task field is assigned.
    if(!value)throw ApplyManagedError("null SecureString initializer");
    return std::move(*value);
}
}
std::int32_t parse_apply_integer(const std::u16string& text,const ApplyNumberSigns& signs) {
    std::size_t pos=0;
    while(pos<text.size() && number_space(text[pos]))++pos;
    bool negative=false;
    if(!signs.positive.empty() && text.compare(pos,signs.positive.size(),signs.positive)==0)pos+=signs.positive.size();
    else if(!signs.negative.empty() && text.compare(pos,signs.negative.size(),signs.negative)==0) {pos+=signs.negative.size();negative=true;}
    const auto first=pos;
    std::uint64_t magnitude=0;
    const std::uint64_t limit=negative?2147483648ULL:2147483647ULL;
    bool overflow=false;
    while(pos<text.size() && text[pos]>=u'0' && text[pos]<=u'9') {
        if(!overflow) {magnitude=magnitude*10+static_cast<std::uint32_t>(text[pos]-u'0');overflow=magnitude>limit;}
        ++pos;
    }
    if(pos==first)throw ApplyNumberError(ApplyNumberFailure::Format);
    while(pos<text.size() && number_space(text[pos]))++pos;
    while(pos<text.size() && text[pos]==0)++pos;
    if(pos!=text.size())throw ApplyNumberError(ApplyNumberFailure::Format); // Syntax takes priority over overflow.
    if(overflow)throw ApplyNumberError(ApplyNumberFailure::Overflow);
    const auto signed_value=negative?-static_cast<std::int64_t>(magnitude):static_cast<std::int64_t>(magnitude);
    return static_cast<std::int32_t>(signed_value);
}
void initialize_apply_task(ApplyTaskState& state,std::shared_ptr<ApplyItemSource> item,
    std::optional<std::u16string> gift,std::int32_t type,bool notice,ApplyGiftHost& host) {
    state.item=std::move(item);
    state.gift=gift?std::move(*gift):std::u16string{};
    state.type=type;state.notice=notice;state.locked=false;
    state.name=std::u16string{};state.product_id.clear();state.provision_type.clear();state.provision_text.clear();state.provision_value=0;
    if(state.item) {
        const auto current_item=[&state] {
            if(!state.item)throw ApplyManagedError("missing item during initialization");
            return state.item;
        };
        state.name=wrap_secure_text(current_item()->name());
        state.product_id=wrap_secure_text(current_item()->product_id());
        state.provision_type=wrap_secure_text(current_item()->provision_type());
        state.provision_text=wrap_secure_text(current_item()->provision_text());
        state.provision_value=current_item()->provision_value();
        return;
    }
    if(state.gift.empty())return;
    if(state.gift.find(u'\t')!=std::u16string::npos) {
        const auto fields=split_form_text(&state.gift);
        const auto& first=fields.at(0);
        state.talk_character=parse_apply_integer(first,host.current_number_signs());
        state.talk_text=fields.at(1);
        return;
    }
    const auto prefix_text=state.gift;
    if(host.starts_special_gift(prefix_text))return;
    const auto product=split_text_color_fields(&state.gift); // Same trimmed comma overload.
    state.product_id=product.at(0);
    if(product.size()>=2)state.name=product.at(1);
    const auto provision=split_trimmed_text_fields(&state.product_id,u'_');
    if(!provision.empty())state.provision_type=provision.at(0);
    if(provision.size()>1)state.provision_text=provision.at(1);
    try {
        if(state.provision_text==u"on")state.provision_value=1;
        else if(state.provision_text==u"off")state.provision_value=0;
        else {
            const auto captured=state.provision_text; // SecureString.Get precedes CurrentInfo lookup.
            const auto signs=host.current_number_signs();
            state.provision_value=parse_apply_integer(captured,signs);
        }
    } catch(const ApplyManagedError&) {
        // Only this provision conversion is protected by Init's native catch.
    }
    if(state.name->empty()) {
        const auto catalog=host.catalog(); // Provider may replace live task fields before enumeration.
        const auto selected=resolve_apply_name_item(catalog,state.provision_type,state.provision_text,state.provision_value);
        if(selected)state.name=wrap_secure_text(selected->name);
    }
}
void initialize_apply_gift(ApplyTaskState& state,std::optional<std::u16string> gift,std::int32_t type,bool notice,ApplyGiftHost& host) {
    initialize_apply_task(state,nullptr,std::move(gift),type,notice,host);
}
std::shared_ptr<NotificationApplyTask> make_notification_gift(NotificationText text,ApplyGiftHost& host) {
    auto state=std::make_shared<ApplyTaskState>();
    initialize_apply_gift(*state,text?std::optional<std::u16string>(*text):std::nullopt,0,true,host);
    return std::make_shared<NotificationApplyTask>(NotificationApplyTask{nullptr,state->gift,state});
}
std::shared_ptr<NotificationApplyTask> make_notification_item(std::shared_ptr<ApplyItemSource> item,std::int32_t type,bool notice,ApplyGiftHost& host) {
    auto state=std::make_shared<ApplyTaskState>();
    initialize_apply_task(*state,std::move(item),std::u16string{},type,notice,host);
    return std::make_shared<NotificationApplyTask>(NotificationApplyTask{state->item,state->gift,state});
}
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
