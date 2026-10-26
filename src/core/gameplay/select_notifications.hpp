#pragma once
#include "event_flow.hpp"
#include <optional>
#include <string>
namespace saga::core {
using NotificationText=std::shared_ptr<const std::u16string>;
struct NotificationApplyTask {
    EventObject item; // ApplyTask.item_ +0x10.
    std::optional<std::u16string> gift; // Decoded SecureString +0x18; missing object stays absent.
    EventObject original; // Adapter-owned task with all original parsed fields.
};
bool notification_task_enabled(const NotificationApplyTask&);
enum class NotificationPattern { Crystal, Movie };
bool matches_notification(const NotificationText&,NotificationPattern);
class SelectNotificationHost {
public:
    virtual ~SelectNotificationHost() = default;
    virtual EventObject notification_manager() = 0;
    virtual std::int32_t value_count(EventObject manager) = 0;
    virtual NotificationText value(EventObject manager,std::int32_t index) = 0;
    // Original String.StartsWith uses the current culture; no ordinal default.
    virtual bool starts_currency_sale(NotificationText) = 0;
    virtual EventObject billing_manager() = 0;
    virtual std::shared_ptr<NotificationApplyTask> make_apply_task(NotificationText) = 0;
    virtual void add_apply_task(EventObject manager,std::shared_ptr<NotificationApplyTask>,bool option) = 0;
    virtual void show_currency_sale(NotificationText) = 0;
    virtual void show_free_movie() = 0;
    virtual void remove_value(EventObject manager,std::int32_t index) = 0; // RemoveValues([index]).
    virtual void save_game(bool automatic,bool option) = 0;
    virtual void update_apply_tasks(EventObject manager,bool option) = 0;
};
void update_select_notifications(SelectNotificationHost&);
}
