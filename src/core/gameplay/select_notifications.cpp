#include "select_notifications.hpp"
#include <stdexcept>
#include <string_view>
namespace saga::core {
namespace {
template<class T> const std::shared_ptr<T>& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("missing notification context");
    return value;
}
}
// The two fixed Regex.IsMatch patterns in SelectForm.UpdateNotifyValue (0x11edb18).
// Default $ also accepts the position before one final LF; input is not trimmed.
bool matches_notification(const NotificationText& text,NotificationPattern pattern) {
    required(text);
    const std::u16string_view prefix=pattern==NotificationPattern::Crystal?u"crystal_":u"movie_";
    std::u16string_view value=*text;
    if(!value.empty() && value.back()==u'\n')value.remove_suffix(1);
    if(value.size()<=prefix.size() || value.substr(0,prefix.size())!=prefix)return false;
    for(const auto c:value.substr(prefix.size())) {
        const bool accepted=pattern==NotificationPattern::Crystal
            ? c>=u'0' && c<=u'9'
            : (c>=u'a' && c<=u'z') || (c>=u'A' && c<=u'Z');
        if(!accepted)return false;
    }
    return true;
}
// ApplyTask.Enable 0x11bbaec; structural readiness, not payment authorization.
bool notification_task_enabled(const NotificationApplyTask& task) {
    return bool(task.item) || (task.gift && !task.gift->empty());
}
// SelectForm.UpdateNotifyValue 0x11edb18. The notification manager is retained across all operations.
void update_select_notifications(SelectNotificationHost& host) {
    const auto manager=host.notification_manager();required(manager);
    const auto count=host.value_count(manager);
    // Managed collection counts are nonnegative; reject invalid decoded providers.
    if(count<0)throw std::invalid_argument("negative notification count");
    bool changed=false;
    for(auto index=count;index>0;) {
        --index;
        const auto text=host.value(manager,index);
        if(matches_notification(text,NotificationPattern::Crystal)) {
            const auto billing=host.billing_manager();
            const auto task=host.make_apply_task(text);required(task);
            if(notification_task_enabled(*task)) {
                required(billing);
                changed=true;
                host.add_apply_task(billing,task,true);
            }
        } else {
            required(text); // Explicit null guard before String.StartsWith.
            if(host.starts_currency_sale(text))host.show_currency_sale(text);
            else if(matches_notification(text,NotificationPattern::Movie))host.show_free_movie();
        }
        host.remove_value(manager,index);
    }
    if(changed) {
        host.save_game(false,true);
        const auto billing=host.billing_manager();required(billing);
        host.update_apply_tasks(billing,true);
    }
}
}
