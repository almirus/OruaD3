#pragma once

#include <string>

namespace orua3d {

/// Convert the compiler's fixed "Mmm dd yyyy" date to an unambiguous ISO date.
/// __DATE__ is evaluated while each executable's main translation unit is built.
inline std::string build_date_iso() {
    static constexpr char kMonths[] =
        "JanFebMarAprMayJunJulAugSepOctNovDec";
    static constexpr char kCompilerDate[] = __DATE__;

    unsigned month = 0u;
    for (unsigned index = 0u; index < 12u; ++index) {
        const char* candidate = kMonths + index * 3u;
        if (candidate[0] == kCompilerDate[0]
            && candidate[1] == kCompilerDate[1]
            && candidate[2] == kCompilerDate[2]) {
            month = index + 1u;
            break;
        }
    }
    const unsigned day =
        (kCompilerDate[4] == ' ' ? 0u
                                : static_cast<unsigned>(kCompilerDate[4] - '0') * 10u)
        + static_cast<unsigned>(kCompilerDate[5] - '0');

    std::string result(kCompilerDate + 7u, 4u);
    result.push_back('-');
    result.push_back(static_cast<char>('0' + month / 10u));
    result.push_back(static_cast<char>('0' + month % 10u));
    result.push_back('-');
    result.push_back(static_cast<char>('0' + day / 10u));
    result.push_back(static_cast<char>('0' + day % 10u));
    return result;
}

} // namespace orua3d
