#include "apply_culture_host.hpp"
#include <exception>
namespace saga::core {
ApplyNumberSigns CultureApplyGiftHost::current_number_signs(){
    try{
        auto format=current_number_format(state_,thread_,current_,numbers_);
        if(!format)throw CultureManagedException("null NumberFormatInfo while reading parser signs");
        return {format->positive,format->negative};
    }catch(const CultureManagedException& error){
        // Init LSDA 0x90f870 catches System.Exception only around provision parsing.
        // Non-Exception managed objects and C++ data/host errors must propagate.
        std::throw_with_nested(ApplyManagedError(error.what()));
    }
}
}
