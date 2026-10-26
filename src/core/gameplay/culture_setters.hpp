#pragma once
#include "current_culture.hpp"
namespace saga::core {
enum class CultureMutationFailure { NullValue,ReadOnly };
class CultureMutationError:public CultureManagedException {
public:
    explicit CultureMutationError(CultureMutationFailure value):CultureManagedException("culture mutation failed"),kind(value){}
    CultureMutationFailure kind;
};
class CulturePopulationHost {
public:
    virtual ~CulturePopulationHost() = default;
    // Native LCID population can update the whole decoded culture, not just identity.
    virtual bool populate(CultureInstance&,std::int32_t lcid) = 0;
};
void set_positive_sign(CultureNumberFormat&,const std::optional<std::u16string>&);
void set_negative_sign(CultureNumberFormat&,const std::optional<std::u16string>&);
void set_culture_number_format(CultureInstance&,std::shared_ptr<CultureNumberFormat>,CulturePopulationHost&);
}
