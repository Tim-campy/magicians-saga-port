#pragma once
#include "apply_task.hpp"
#include "current_culture.hpp"
#include <cstddef>
#include <string_view>
namespace saga::core {
std::u16string decode_culture_utf8(std::string_view);
// Validated extracted data; culture selection remains the caller's responsibility.
ApplyNumberSigns read_apply_number_signs(std::string_view records,std::string_view pool,std::size_t index);
// Required virtual GetFormat and CultureData-backed construction boundaries.
class CultureNumberHost {
public:
    virtual ~CultureNumberHost() = default;
    virtual std::shared_ptr<CultureNumberFormat> construct_number_format(const CultureInstance&) = 0;
    virtual std::shared_ptr<CultureNumberFormat> get_format(CultureInstance&) = 0;
};
std::shared_ptr<CultureNumberFormat> culture_number_format(CultureInstance&,CultureNumberHost&);
std::shared_ptr<CultureNumberFormat> current_number_format(CurrentCultureState&,ThreadCultureHost&,
    CurrentCultureHost&,CultureNumberHost&);
// Validated CultureData projection; caller supplies the original invariant-mode gate.
std::shared_ptr<CultureNumberFormat> read_culture_number_format(const CultureInstance&,
    bool invariant_mode,std::string_view records,std::string_view pool);
}
