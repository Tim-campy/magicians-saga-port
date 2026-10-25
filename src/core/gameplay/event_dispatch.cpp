#include "event_dispatch.hpp"

namespace saga::core {
namespace {
std::int32_t signed32(std::uint32_t bits) {
    return bits <= 0x7fffffffU ? static_cast<std::int32_t>(bits)
        : -1 - static_cast<std::int32_t>(0xffffffffU - bits);
}
std::int64_t signed64(std::uint64_t bits) {
    return bits <= 0x7fffffffffffffffULL ? static_cast<std::int64_t>(bits)
        : -1 - static_cast<std::int64_t>(0xffffffffffffffffULL - bits);
}
}
std::int32_t stamina_limit_from_item_state(std::int32_t state) {
    // GetStaminaMax34465, ELF0x110d3fc; only zero versus nonzero is tested.
    return state == 0 ? 10 : 20;
}
std::int64_t event_stamina_balance(std::int64_t balance, std::int32_t percent,
                                  std::int32_t percent_limit, std::int32_t clamp_limit) {
    const auto product = signed32(static_cast<std::uint32_t>(percent) *
                                  static_cast<std::uint32_t>(percent_limit));
    const auto delta = product / 100; // Signed division truncates toward zero.
    auto value = signed64(static_cast<std::uint64_t>(balance) + static_cast<std::uint64_t>(delta));
    // Original CMP uses zero-extension of the second limit. GetStaminaMax only
    // returns10/20, but preserve the instruction's width explicitly.
    const auto upper = static_cast<std::int64_t>(static_cast<std::uint32_t>(clamp_limit));
    if (value > upper) value = upper;
    return value < 0 ? 0 : value;
}
EventDispatchResult dispatch_known_event_instruction(EventInstruction row,
                                                    EventContext context,
                                                    EventEffectsHost& host) {
    // Unlike the matrix executor, the unit dispatcher tolerates null/empty rows.
    if (!row || row->empty()) return {true, {}};
    const auto resolved = resolve_event_arguments(*row, context.argi);
    switch (resolved[0]) {
    case 1: case 2: case 4: case 15:
        apply_event_progression(resolved, host.progression());
        return {true, {}};
    case 8: case 12: {
        const auto id = resolved.at(1);
        auto form = resolved[0] == 8 ? host.make_talk_form(id, context.args)
                                     : host.make_event_form(id, context.args);
        host.push_form(form);
        return {true, form};
    }
    case 10: return {true, {}};
    case 31: {
        // Original reads first limit BEFORE checking the percent argument.
        const auto percent_limit = host.stamina_limit();
        const auto percent = resolved.at(1);
        const auto balance = host.read_stamina();
        const auto clamp_limit = host.stamina_limit();
        host.write_stamina(event_stamina_balance(balance, percent, percent_limit, clamp_limit));
        return {true, {}};
    }
    default: return {false, {}};
    }
}
}
