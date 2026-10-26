#include "select_notifications.hpp"
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> const std::shared_ptr<T>& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("missing notification context");
    return value;
}
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
        if(host.matches(text,NotificationPattern::Crystal)) {
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
            else if(host.matches(text,NotificationPattern::Movie))host.show_free_movie();
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
