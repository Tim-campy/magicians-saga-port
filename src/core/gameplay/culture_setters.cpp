#include "culture_setters.hpp"
namespace saga::core {
namespace {
void validate_sign(const CultureNumberFormat& format,const std::optional<std::u16string>& value){
    if(!value)throw CultureMutationError(CultureMutationFailure::NullValue);
    if(format.read_only)throw CultureMutationError(CultureMutationFailure::ReadOnly);
}
}
// NumberFormatInfo.set_PositiveSign 0x1e159c0 and set_NegativeSign 0x1e155a0.
void set_positive_sign(CultureNumberFormat& format,const std::optional<std::u16string>& value){
    validate_sign(format,value);format.positive=*value;
}
void set_negative_sign(CultureNumberFormat& format,const std::optional<std::u16string>& value){
    validate_sign(format,value);format.negative=*value;
}
// CultureInfo.set_NumberFormat 0x1e26fd0. The false population result is ignored.
void set_culture_number_format(CultureInstance& culture,std::shared_ptr<CultureNumberFormat> value,CulturePopulationHost& host){
    if(!culture.identity.constructed){
        const auto lcid=culture.identity.lcid;
        host.populate(culture,lcid);
        culture.identity.constructed=true;
    }
    if(culture.read_only)throw CultureMutationError(CultureMutationFailure::ReadOnly);
    if(!value)throw CultureMutationError(CultureMutationFailure::NullValue);
    culture.number_format=std::move(value);
}
}
