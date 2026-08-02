#include "error_quantization.hpp"

#include <limits>

namespace auro3d::encode {

bool pack_golomb_error(
    std::int32_t value,
    std::uint32_t bit_width,
    std::uint32_t& packed) {
    packed = 0;
    if (bit_width == 0u || bit_width > 32u)
        return false;
    if (bit_width == 32u) {
        packed = static_cast<std::uint32_t>(value);
        return true;
    }
    const std::uint32_t magnitude_limit = (std::uint32_t{1} << (bit_width - 1u)) - 1u;
    const std::uint32_t magnitude = value < 0
        ? static_cast<std::uint32_t>(-static_cast<std::int64_t>(value))
        : static_cast<std::uint32_t>(value);
    if (magnitude > magnitude_limit)
        return false;
    packed = magnitude;
    if (value < 0)
        packed |= std::uint32_t{1} << (bit_width - 1u);
    return true;
}

bool unpack_golomb_error(
    std::uint32_t packed,
    std::uint32_t bit_width,
    std::int32_t& value) {
    value = 0;
    if (bit_width == 0u || bit_width > 32u)
        return false;
    if (bit_width == 32u) {
        value = static_cast<std::int32_t>(packed);
        return true;
    }
    const std::uint32_t sign_bit = std::uint32_t{1} << (bit_width - 1u);
    const std::uint32_t magnitude = packed & (sign_bit - 1u);
    value = (packed & sign_bit) == 0u
        ? static_cast<std::int32_t>(magnitude)
        : -static_cast<std::int32_t>(magnitude);
    return true;
}

bool pack_golomb_errors(
    const std::vector<std::int32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed,
    std::string& error) {
    error.clear();
    packed.clear();
    if (bit_width == 0u || bit_width > 32u) {
        error = "residual code width is outside the native range";
        return false;
    }
    packed.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
        std::uint32_t encoded = 0u;
        if (!pack_golomb_error(values[index], bit_width, encoded)) {
            error = "residual at index " + std::to_string(index)
                + " does not fit the selected code width";
            packed.clear();
            return false;
        }
        std::int32_t roundtrip = 0;
        if (!unpack_golomb_error(encoded, bit_width, roundtrip)
            || roundtrip != values[index]) {
            error = "residual at index " + std::to_string(index)
                + " failed native sign/magnitude round-trip";
            packed.clear();
            return false;
        }
        packed.push_back(encoded);
    }
    return true;
}

} // namespace auro3d::encode
