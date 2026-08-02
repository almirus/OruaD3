#include "mix_mix3.hpp"

#include "mix_deltas.hpp"

namespace auro3d::encode {
namespace {

std::int32_t add_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

std::int32_t sub_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right));
}

std::int32_t mul_wrap(std::int32_t value, std::int32_t factor) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(value) * static_cast<std::uint32_t>(factor));
}

std::int32_t mix3_round4(std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    if (value < 0) {
        const std::uint32_t magnitude =
            (std::uint32_t{1} - raw) & 0xFFFFFFFCu;
        return static_cast<std::int32_t>(std::uint32_t{0} - magnitude);
    }
    return static_cast<std::int32_t>(
        (raw + std::uint32_t{1}) & 0x7FFFFFFCu);
}

std::int32_t residual_component(
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::uint64_t index,
    std::size_t component,
    std::string& error) {
    if (index >= residuals.size() || component >= 2u) {
        error = "mix3 mixer residual index is out of range";
        return 0;
    }
    return residuals[static_cast<std::size_t>(index)][component];
}

bool mix3_mix0(
    const std::vector<std::int32_t>& source,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::vector<std::int32_t>& carrier,
    std::string& error) {
    const std::size_t n = source.size();
    const std::size_t groups = (n - 1u) / 3u;
    std::int32_t previous = mix3_round4(source[0]);
    for (std::size_t group = 0; group < groups; ++group) {
        const std::size_t sample = 3u * group + 3u;
        const std::int32_t current = mix3_round4(source[sample]);
        const std::int32_t delta = sub_wrap(current, previous);
        const std::int32_t adjusted = delta >= 0 ? delta : add_wrap(delta, 3);
        const std::int32_t quotient = codec_v3_arithmetic_shift_right(adjusted, 2u);
        const std::uint64_t first_index = indices[3u * group + 1u];
        const std::uint64_t second_index = indices[3u * group + 2u];
        const std::int32_t first = residual_component(residuals, first_index, 0u, error);
        if (!error.empty())
            return false;
        const std::int32_t second = residual_component(residuals, second_index, 0u, error);
        if (!error.empty())
            return false;
        carrier[3u * group + 1u] = add_wrap(first, add_wrap(quotient, previous));
        carrier[3u * group + 2u] = sub_wrap(add_wrap(second, current), quotient);
        carrier[3u * group + 3u] = current;
        previous = current;
    }
    const std::size_t remainder = n - 1u - groups * 3u;
    const std::size_t base = groups * 3u + 1u;
    if (remainder == 2u) {
        const std::int32_t delta = sub_wrap(source[base + 1u], previous);
        const std::int32_t quotient = delta / 3;
        const std::int32_t first = residual_component(
            residuals, indices[base], 0u, error);
        if (!error.empty())
            return false;
        const std::int32_t second = residual_component(
            residuals, indices[base + 1u], 0u, error);
        if (!error.empty())
            return false;
        carrier[base] = add_wrap(first, add_wrap(quotient, previous));
        carrier[base + 1u] = add_wrap(second, add_wrap(previous, mul_wrap(quotient, 3)));
    } else if (remainder == 1u) {
        carrier[base] = source[base];
    } else if (remainder != 0u) {
        error = "mix3 primary tail shape is invalid";
        return false;
    }
    return true;
}

bool mix3_mix1_or_mix2(
    const std::vector<std::int32_t>& source,
    std::size_t phase,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::vector<std::int32_t>& carrier,
    std::string& error) {
    const std::size_t n = source.size();
    const std::size_t first_component = 1u;
    const std::size_t second_component = phase == 1u ? 0u : 1u;
    const std::size_t groups = (n - phase - 1u) / 3u;
    std::int32_t previous = mix3_round4(source[phase]);
    for (std::size_t group = 0; group < groups; ++group) {
        const std::size_t sample = phase + 3u + 3u * group;
        const std::int32_t current = mix3_round4(source[sample]);
        const std::int32_t delta = sub_wrap(current, previous);
        const std::int32_t adjusted = delta >= 0 ? delta : add_wrap(delta, 3);
        const std::int32_t quotient = codec_v3_arithmetic_shift_right(adjusted, 2u);
        const std::size_t carrier_base = phase + 1u + 3u * group;
        const std::size_t index_base = phase + 1u + 3u * group;
        const std::int32_t first = residual_component(
            residuals, indices[index_base], first_component, error);
        if (!error.empty())
            return false;
        const std::int32_t second = residual_component(
            residuals, indices[index_base + 1u], second_component, error);
        if (!error.empty())
            return false;
        carrier[carrier_base] = add_wrap(
            carrier[carrier_base], add_wrap(first, add_wrap(quotient, previous)));
        carrier[carrier_base + 1u] = add_wrap(
            carrier[carrier_base + 1u], sub_wrap(add_wrap(second, current), quotient));
        carrier[carrier_base + 2u] = add_wrap(carrier[carrier_base + 2u], current);
        previous = current;
    }
    const std::size_t remainder = n - phase - 1u - groups * 3u;
    const std::size_t carrier_base = phase + 1u + groups * 3u;
    const std::size_t index_base = phase + 1u + groups * 3u;
        const std::size_t source_base = phase + groups * 3u;
    if (remainder == 2u) {
        const std::int32_t delta = sub_wrap(source[source_base + 2u], previous);
        const std::int32_t quotient = delta / 3;
        const std::int32_t first = residual_component(
            residuals, indices[index_base], first_component, error);
        if (!error.empty())
            return false;
        const std::int32_t second = residual_component(
            residuals, indices[index_base + 1u], second_component, error);
        if (!error.empty())
            return false;
        carrier[carrier_base] = add_wrap(
            carrier[carrier_base], add_wrap(first, add_wrap(quotient, previous)));
        carrier[carrier_base + 1u] = add_wrap(
            carrier[carrier_base + 1u], add_wrap(second, add_wrap(previous, mul_wrap(quotient, 3))));
    } else if (remainder == 1u) {
        carrier[carrier_base] = add_wrap(carrier[carrier_base], source[source_base + 1u]);
    } else if (remainder != 0u) {
        error = "mix3 source tail shape is invalid";
        return false;
    }
    return true;
}

} // namespace

bool unpack_mix3_residual_table(
    const std::vector<std::int64_t>& packed,
    std::vector<std::array<std::int32_t, 2>>& residuals,
    std::string& error) {
    error.clear();
    residuals.clear();
    if (packed.size() > residuals.max_size()) {
        error = "mix3 residual table exceeds storage capacity";
        return false;
    }
    residuals.reserve(packed.size());
    for (const std::int64_t value : packed) {
        const std::uint64_t raw = static_cast<std::uint64_t>(value);
        residuals.push_back({
            static_cast<std::int32_t>(static_cast<std::uint32_t>(raw)),
            static_cast<std::int32_t>(static_cast<std::uint32_t>(raw >> 32u))});
    }
    return true;
}

bool pack_mix3_residual_table(
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::vector<std::int64_t>& packed,
    std::string& error) {
    error.clear();
    packed.clear();
    if (residuals.size() > packed.max_size()) {
        error = "mix3 packed residual table exceeds storage capacity";
        return false;
    }
    packed.reserve(residuals.size());
    for (const auto& value : residuals) {
        const std::uint64_t low = static_cast<std::uint32_t>(value[0]);
        const std::uint64_t high = static_cast<std::uint32_t>(value[1]);
        packed.push_back(static_cast<std::int64_t>(low | (high << 32u)));
    }
    return true;
}

bool mix3_mixer_reconstruct(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::vector<std::int32_t>& carrier,
    Mix3MixerSeeds& seeds,
    std::string& error) {
    error.clear();
    carrier.clear();
    seeds = {};
    if (primary.size() < 6u || primary.size() != secondary.size()
        || primary.size() != tertiary.size() || indices.size() != primary.size()) {
        error = "mix3 mixer requires matching source and index vectors of length >= 6";
        return false;
    }
    carrier.assign(primary.size(), 0);
    if (!mix3_mix0(primary, indices, residuals, carrier, error)
        || !mix3_mix1_or_mix2(secondary, 1u, indices, residuals, carrier, error)
        || !mix3_mix1_or_mix2(tertiary, 2u, indices, residuals, carrier, error)) {
        return false;
    }

    carrier[0] = add_wrap(
        mix3_round4(primary[0]), add_wrap(secondary[0], tertiary[0]));
    carrier[1] = add_wrap(
        carrier[1],
        add_wrap(mix3_round4(secondary[1]), tertiary[1]));
    carrier[2] = add_wrap(carrier[2], mix3_round4(tertiary[2]));

    const std::int32_t primary0 = mix3_round4(primary[0]);
    const std::int32_t secondary1 = mix3_round4(secondary[1]);
    const std::int32_t tertiary2 = mix3_round4(tertiary[2]);
    const std::int32_t r00 = residual_component(residuals, indices[0], 0u, error);
    if (!error.empty())
        return false;
    const std::int32_t r01 = residual_component(residuals, indices[0], 1u, error);
    if (!error.empty())
        return false;
    const std::int32_t r10 = residual_component(residuals, indices[1], 0u, error);
    if (!error.empty())
        return false;
    const std::int32_t r11 = residual_component(residuals, indices[1], 1u, error);
    if (!error.empty())
        return false;
    const std::int32_t r20 = residual_component(residuals, indices[2], 0u, error);
    if (!error.empty())
        return false;
    const std::int32_t r21 = residual_component(residuals, indices[2], 1u, error);
    if (!error.empty())
        return false;
    seeds.values[0] = sub_wrap(secondary[0], r00);
    // auro::mix::mix3 @ 0x4EE870 stores the two components selected by
    // indices[0] in seed slots 0/1. Using the first component selected by
    // indices[1] here corrupts the decoder's initial extrapolate state.
    seeds.values[1] = sub_wrap(tertiary[0], r01);
    seeds.values[2] = sub_wrap(carrier[1], add_wrap(tertiary[1], add_wrap(secondary1, r10)));
    seeds.values[3] = sub_wrap(tertiary[1], r11);
    const std::int32_t seed4_base = sub_wrap(
        add_wrap(carrier[2], add_wrap(mul_wrap(seeds.values[2], -3), mul_wrap(primary0, 2))),
        add_wrap(r21, add_wrap(tertiary2, r20)));
    seeds.values[4] = seed4_base;
    return true;
}

} // namespace auro3d::encode
