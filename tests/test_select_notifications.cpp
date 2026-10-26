#include "core/gameplay/select_notifications.hpp"
#include "core/gameplay/apply_task.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"notifications contract failure\n";std::exit(1);}}
template<class F> void fails(F f){try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectNotificationHost,ApplyGiftHost {
    EventObject manager=std::make_shared<int>(1),billing=std::make_shared<int>(2);
    std::vector<NotificationText> values;
    std::vector<std::string> log;
    bool disabled=false,fail_add=false,fail_save=false;
    std::shared_ptr<NotificationApplyTask> constructed;
    ApplyNumberSigns current_number_signs() override {return {u"+",u"-"};}
    bool starts_special_gift(const std::u16string& text) override {return text.front()==u'@';}
    std::shared_ptr<ApplyCatalog> catalog() override {return std::make_shared<ApplyCatalog>();}
    EventObject notification_manager() override {log.push_back("manager");return manager;}
    std::int32_t value_count(EventObject m) override {check(m==manager);log.push_back("count");return static_cast<std::int32_t>(values.size());}
    NotificationText value(EventObject m,std::int32_t i) override {check(m==manager);log.push_back("value"+std::to_string(i));return values.at(static_cast<std::size_t>(i));}
    bool starts_currency_sale(NotificationText s) override {return *s==u"sale";}
    EventObject billing_manager() override {log.push_back("billing");return billing;}
    std::shared_ptr<NotificationApplyTask> make_apply_task(NotificationText text) override {log.push_back("make");constructed=make_notification_gift(disabled?nullptr:text,*this);return constructed;}
    void add_apply_task(EventObject b,std::shared_ptr<NotificationApplyTask>,bool option) override {check(b==billing && option);log.push_back("add");if(fail_add)throw std::runtime_error("add");}
    void show_currency_sale(NotificationText) override {log.push_back("sale");}
    void show_free_movie() override {log.push_back("movie");}
    void remove_value(EventObject m,std::int32_t i) override {check(m==manager);log.push_back("remove"+std::to_string(i));values.erase(values.begin()+i);}
    void save_game(bool a,bool b) override {check(!a && b);log.push_back("save");if(fail_save)throw std::runtime_error("save");}
    void update_apply_tasks(EventObject b,bool option) override {check(b==billing && option);log.push_back("update");}
    void put(const char16_t* s){values.push_back(std::make_shared<const std::u16string>(s));}
};
int main(){
    const auto match=[](std::u16string s,NotificationPattern p){return matches_notification(std::make_shared<const std::u16string>(std::move(s)),p);};
    const auto crystal=NotificationPattern::Crystal,movie=NotificationPattern::Movie;
    fails([&]{matches_notification(nullptr,crystal);});
    fails([&]{matches_notification(nullptr,movie);});
    for(const auto* s:{u"crystal_0",u"crystal_0123456789",u"crystal_42\n"})check(match(s,crystal));
    for(const auto* s:{u"movie_aZ",u"movie_A\n"})check(match(s,movie));
    for(const auto* s:{u"",u"crystal_",u"crystal_\n",u"Crystal_1",u" crystal_1",u"crystal_1 ",u"crystal_1\r\n",u"crystal_1\n\n",u"crystal_1\nx",u"crystal_+1",u"crystal_\uff11",u"\ncrystal_1"})check(!match(s,crystal));
    for(const auto* s:{u"",u"movie_",u"movie_\n",u"Movie_A",u"movie_1",u"movie_\u00e9",u"movie_A\r\n",u"movie_A\n\n",u"movie_A\nx",u"movie_A ",u"\nmovie_A"})check(!match(s,movie));
    check(!match(std::u16string(u"crystal_1\0",10),crystal));
    check(!match(std::u16string(u"movie_A\0",8),movie));
    // All UTF-16 code units, including lone surrogates, at the character-class boundary.
    for(std::uint32_t i=0;i<=0xffff;++i){
        const auto c=static_cast<char16_t>(i);
        check(match(std::u16string(u"crystal_")+c,crystal)==(i>=48 && i<=57));
        check(match(std::u16string(u"movie_")+c,movie)==((i>=65 && i<=90)||(i>=97 && i<=122)));
    }
    {Host h;h.put(u"crystal_5\n");h.put(u"movie_Ab\n");update_select_notifications(h);auto task=std::static_pointer_cast<ApplyTaskState>(h.constructed->original);check(task->gift==u"crystal_5\n" && task->provision_value==5 && h.values.empty());}

    {Host h;h.put(u"crystal_5");update_select_notifications(h);auto task=std::static_pointer_cast<ApplyTaskState>(h.constructed->original);check(task->provision_type==u"crystal" && task->provision_value==5 && h.values.empty() && h.log.back()=="update");}

    check(!notification_task_enabled({nullptr,std::nullopt,nullptr}));
    check(notification_task_enabled({std::make_shared<int>(1),std::nullopt,nullptr}));
    check(!notification_task_enabled({nullptr,u"",nullptr}));
    {Host h;h.put(u"crystal_5");h.put(u"sale");h.put(u"movie_Ab");h.put(u"unknown");update_select_notifications(h);check(h.values.empty());check((h.log==std::vector<std::string>{"manager","count","value3","remove3","value2","movie","remove2","value1","sale","remove1","value0","billing","make","add","remove0","save","billing","update"}));}
    {Host h;h.put(u"crystal_5");h.disabled=true;h.billing.reset();update_select_notifications(h);check(h.values.empty() && h.log.back()=="remove0");}
    {Host h;h.put(u"crystal_5");h.put(u"unknown");h.fail_add=true;fails([&]{update_select_notifications(h);});check(h.values.size()==1 && h.log.back()=="add");}
    {Host h;h.put(u"crystal_5");h.fail_save=true;fails([&]{update_select_notifications(h);});check(h.values.empty() && h.log.back()=="save");}
    {Host h;h.values.push_back(nullptr);fails([&]{update_select_notifications(h);});check(h.values.size()==1);}
    {Host h;h.manager.reset();fails([&]{update_select_notifications(h);});check(h.log.size()==1);}
}
