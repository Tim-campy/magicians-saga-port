#pragma once
#include "apply_culture.hpp"
namespace saga::core {
class ApplyGiftContentHost {
public:
    virtual ~ApplyGiftContentHost() = default;
    virtual bool starts_special_gift(const std::u16string&) = 0;
    virtual std::shared_ptr<ApplyCatalog> catalog() = 0;
};
// Binds the recovered CurrentInfo path without supplying guessed locale, prefix
// collation, catalog or thread state. Dependencies must outlive this adapter.
class CultureApplyGiftHost:public ApplyGiftHost {
public:
    CultureApplyGiftHost(CurrentCultureState& state,ThreadCultureHost& thread,
        CurrentCultureHost& current,CultureNumberHost& numbers,ApplyGiftContentHost& content)
        :state_(state),thread_(thread),current_(current),numbers_(numbers),content_(content){}
    ApplyNumberSigns current_number_signs()override;
    bool starts_special_gift(const std::u16string& value)override{return content_.starts_special_gift(value);}
    std::shared_ptr<ApplyCatalog> catalog()override{return content_.catalog();}
private:
    CurrentCultureState& state_;
    ThreadCultureHost& thread_;
    CurrentCultureHost& current_;
    CultureNumberHost& numbers_;
    ApplyGiftContentHost& content_;
};
}
