#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace saga::core {
class CultureLocaleHost {
public:
    virtual ~CultureLocaleHost() = default;
    // Original Android source: Locale.getDefault().toLanguageTag().
    virtual std::string platform_locale() = 0;
    virtual std::optional<std::string> environment(std::string_view name) = 0;
    // Original setlocale(LC_ALL, nullptr): query only, no locale mutation.
    virtual std::optional<std::string> process_locale() = 0;
};
std::string select_native_locale(CultureLocaleHost&);
std::string normalize_culture_locale(std::string_view);
}
