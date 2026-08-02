#pragma once

#include <string>

namespace auro3d_encode {

constexpr char kVersion[] = "0.0.1_alfa";
constexpr char kName[] = "orua3d-encode";
constexpr char kAuthor[] = "@almirus";

inline std::string make_encode_comment() {
    return std::string("Encoded by ") + kName + " " + kVersion
        + ", author " + kAuthor;
}

} // namespace auro3d_encode
