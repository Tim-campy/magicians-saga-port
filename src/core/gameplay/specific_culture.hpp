#pragma once
#include "current_culture.hpp"
#include <vector>
namespace saga::core {
struct NeutralCultureEntry {std::u16string name;std::int32_t lcid;};
struct CultureTables {
    std::string_view names,records,pool;
    const std::vector<NeutralCultureEntry>& neutral; // Validated unique extracted keys.
};
enum class CultureSelectionFailure { NullName,NameNotFound,UnknownNeutral,NegativeLcid,MissingLcid };
class CultureSelectionError:public CultureManagedError {
public:
    explicit CultureSelectionError(CultureSelectionFailure value):CultureManagedError("culture selection failed"),kind(value){}
    CultureSelectionFailure kind;
};
// Full CultureData/calendar allocation is outside this decoded number-selection model.
// The host owns the invariant number-format singleton and implements Unicode lowercasing.
class SpecificCultureHost {
public:
    virtual ~SpecificCultureHost() = default;
    virtual std::u16string lower_invariant(const std::u16string&) = 0;
    virtual std::shared_ptr<CultureInstance> invariant_culture() = 0;
    virtual std::shared_ptr<CultureNumberFormat> invariant_number_format() = 0;
};
std::shared_ptr<CultureInstance> create_culture_by_lcid(std::int32_t,bool user_override,bool read_only,
    const CultureTables&,SpecificCultureHost&);
std::shared_ptr<CultureInstance> create_specific_culture(const std::optional<std::u16string>&,
    const CultureTables&,SpecificCultureHost&);
}
