#pragma once
#include "core/gameplay/specific_culture.hpp"
#include "core/gameplay/apply_culture.hpp"
#include "core/gameplay/culture_lower.hpp"
#include <filesystem>
namespace saga::desktop {
// Strict schema-1 decoder; rejects unknown/duplicate fields and duplicate names.
std::vector<core::NeutralCultureEntry> parse_neutral_cultures(std::string_view);
class CultureData {
public:
    static CultureData load(const std::filesystem::path& extracted_root);
    core::CultureTables tables()const{return {names_,records_,pool_,neutral_};}
    core::CultureLowerTables lowercase_tables()const;
    std::string_view number_records()const{return numbers_;}
private:
    CultureData() = default;
    std::string names_,records_,pool_,numbers_;
    std::array<std::string,9> lower_;
    std::vector<core::NeutralCultureEntry> neutral_;
};
// Data must outlive the host. Locale/thread selection stays with the caller.
class CultureDataHost:public core::SpecificCultureHost,public core::CultureNumberHost {
public:
    explicit CultureDataHost(const CultureData& data):data_(data){}
    CultureDataHost(CultureData&&) = delete;
    std::u16string lower_invariant(const std::u16string&)override;
    std::shared_ptr<core::CultureInstance> invariant_culture()override{return invariant_.instance;}
    std::shared_ptr<core::CultureNumberFormat> invariant_number_format()override{return invariant_.number_format;}
    std::shared_ptr<core::CultureNumberFormat> construct_number_format(const core::CultureInstance&)override;
    std::shared_ptr<core::CultureNumberFormat> get_format(core::CultureInstance&)override;
    std::shared_ptr<core::CultureInstance> specific(const std::optional<std::u16string>&);
private:
    const CultureData& data_;
    core::InvariantCultureState invariant_;
};
}
