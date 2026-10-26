#include "core/gameplay/select_notifications.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"notifications contract failure\n";std::exit(1);}}
template<class F> void fails(F f){try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectNotificationHost {
    EventObject manager=std::make_shared<int>(1),billing=std::make_shared<int>(2);
    std::vector<NotificationText> values;
    std::vector<std::string> log;
    bool disabled=false,fail_add=false,fail_save=false;
    EventObject notification_manager() override {log.push_back("manager");return manager;}
    std::int32_t value_count(EventObject m) override {check(m==manager);log.push_back("count");return static_cast<std::int32_t>(values.size());}
    NotificationText value(EventObject m,std::int32_t i) override {check(m==manager);log.push_back("value"+std::to_string(i));return values.at(static_cast<std::size_t>(i));}
    // Synthetic route tokens test the controller, not .NET matching semantics.
    bool matches(NotificationText s,NotificationPattern p) override {if(!s)throw std::invalid_argument("null regex text");return *s==(p==NotificationPattern::Crystal?u"task":u"movie");}
    bool starts_currency_sale(NotificationText s) override {return *s==u"sale";}
    EventObject billing_manager() override {log.push_back("billing");return billing;}
    std::shared_ptr<NotificationApplyTask> make_apply_task(NotificationText) override {log.push_back("make");return std::make_shared<NotificationApplyTask>(NotificationApplyTask{nullptr,disabled?u"":u"synthetic",nullptr});}
    void add_apply_task(EventObject b,std::shared_ptr<NotificationApplyTask>,bool option) override {check(b==billing && option);log.push_back("add");if(fail_add)throw std::runtime_error("add");}
    void show_currency_sale(NotificationText) override {log.push_back("sale");}
    void show_free_movie() override {log.push_back("movie");}
    void remove_value(EventObject m,std::int32_t i) override {check(m==manager);log.push_back("remove"+std::to_string(i));values.erase(values.begin()+i);}
    void save_game(bool a,bool b) override {check(!a && b);log.push_back("save");if(fail_save)throw std::runtime_error("save");}
    void update_apply_tasks(EventObject b,bool option) override {check(b==billing && option);log.push_back("update");}
    void put(const char16_t* s){values.push_back(std::make_shared<const std::u16string>(s));}
};
int main(){
    check(!notification_task_enabled({nullptr,std::nullopt,nullptr}));
    check(notification_task_enabled({std::make_shared<int>(1),std::nullopt,nullptr}));
    check(!notification_task_enabled({nullptr,u"",nullptr}));
    {Host h;h.put(u"task");h.put(u"sale");h.put(u"movie");h.put(u"unknown");update_select_notifications(h);check(h.values.empty());check((h.log==std::vector<std::string>{"manager","count","value3","remove3","value2","movie","remove2","value1","sale","remove1","value0","billing","make","add","remove0","save","billing","update"}));}
    {Host h;h.put(u"task");h.disabled=true;h.billing.reset();update_select_notifications(h);check(h.values.empty() && h.log.back()=="remove0");}
    {Host h;h.put(u"task");h.put(u"unknown");h.fail_add=true;fails([&]{update_select_notifications(h);});check(h.values.size()==1 && h.log.back()=="add");}
    {Host h;h.put(u"task");h.fail_save=true;fails([&]{update_select_notifications(h);});check(h.values.empty() && h.log.back()=="save");}
    {Host h;h.values.push_back(nullptr);fails([&]{update_select_notifications(h);});check(h.values.size()==1);}
    {Host h;h.manager.reset();fails([&]{update_select_notifications(h);});check(h.log.size()==1);}
}
