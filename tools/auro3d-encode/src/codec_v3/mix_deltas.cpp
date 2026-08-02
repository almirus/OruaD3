#include "mix_deltas.hpp"

#include <limits>

namespace auro3d::encode {
namespace {

std::int32_t round4(std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    if (value < 0) {
        const std::uint32_t magnitude = (1u - raw) & 0xFFFFFFFCu;
        return static_cast<std::int32_t>(0u - magnitude);
    }
    return static_cast<std::int32_t>((raw + 1u) & 0x7FFFFFFCu);
}

std::int32_t add_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

std::int32_t sub_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right));
}

bool mix3_delta_024(
    const std::vector<std::int32_t>& samples,
    std::vector<std::int32_t>& out,
    std::size_t offset) {
    const std::size_t count = samples.size();
    std::size_t groups = (count - 1u) / 3u;
    std::int32_t previous = round4(samples[0]);
    std::size_t index = 3u;
    for (std::size_t group = 0; group < groups; ++group, index += 3u) {
        const std::int32_t next = round4(samples[index]);
        const std::int32_t delta = sub_wrap(next, previous);
        const std::int32_t step = delta >= 0 ? delta : add_wrap(delta, 3);
        const std::int32_t quotient = codec_v3_arithmetic_shift_right(step, 2u);
        out[offset + 6u * group] = sub_wrap(
            samples[index + 1u], add_wrap(quotient, previous));
        out[offset + 6u * group + 2u] = sub_wrap(
            add_wrap(quotient, samples[index + 2u]), next);
        previous = next;
    }
    const std::size_t remainder = count - 3u * groups - 1u;
    const std::size_t tail = 1u + 3u * groups;
    const std::size_t output = offset + 6u * groups;
    if (remainder == 2u) {
        const std::int32_t difference = sub_wrap(samples[tail + 1u], previous);
        out[output] = sub_wrap(
            samples[tail], add_wrap(difference / 3, previous));
        out[output + 2u] = difference % 3;
    } else if (remainder == 1u) {
        out[output] = 0;
    } else if (remainder != 0u) {
        return false;
    }
    return true;
}

bool mix3_delta_115(
    const std::vector<std::int32_t>& samples,
    std::vector<std::int32_t>& out,
    std::size_t offset) {
    const std::size_t count = samples.size();
    const std::size_t groups = (count - 2u) / 3u;
    std::int32_t previous = round4(samples[1]);
    std::size_t index = 4u;
    for (std::size_t group = 0; group < groups; ++group, index += 3u) {
        const std::int32_t next = round4(samples[index]);
        const std::int32_t delta = sub_wrap(next, previous);
        const std::int32_t step = delta >= 0 ? delta : add_wrap(delta, 3);
        const std::int32_t quotient = codec_v3_arithmetic_shift_right(step, 2u);
        out[offset + 6u * group] = sub_wrap(
            samples[index - 2u], add_wrap(quotient, previous));
        out[offset + 6u * group + 1u] = sub_wrap(
            add_wrap(quotient, samples[index - 1u]), next);
        previous = next;
    }
    const std::size_t remainder = count - 3u * groups - 2u;
    const std::size_t tail = 2u + 3u * groups;
    const std::size_t output = offset + 6u * groups;
    if (remainder == 2u) {
        const std::int32_t difference = sub_wrap(samples[tail + 1u], previous);
        out[output] = sub_wrap(
            samples[tail], add_wrap(difference / 3, previous));
        out[output + 1u] = difference % 3;
    } else if (remainder == 1u) {
        out[output] = 0;
    } else if (remainder != 0u) {
        return false;
    }
    return true;
}

bool mix3_delta_224(
    const std::vector<std::int32_t>& samples,
    std::vector<std::int32_t>& out,
    std::size_t offset) {
    const std::size_t count = samples.size();
    const std::size_t groups = (count - 3u) / 3u;
    std::int32_t previous = round4(samples[2]);
    std::size_t index = 5u;
    for (std::size_t group = 0; group < groups; ++group, index += 3u) {
        const std::int32_t next = round4(samples[index]);
        const std::int32_t delta = sub_wrap(next, previous);
        const std::int32_t step = delta >= 0 ? delta : add_wrap(delta, 3);
        const std::int32_t quotient = codec_v3_arithmetic_shift_right(step, 2u);
        out[offset + 6u * group] = sub_wrap(
            samples[index - 2u], add_wrap(quotient, previous));
        out[offset + 6u * group + 2u] = sub_wrap(
            add_wrap(quotient, samples[index - 1u]), next);
        previous = next;
    }
    const std::size_t remainder = count - 3u * groups - 3u;
    const std::size_t tail = 3u + 3u * groups;
    const std::size_t output = offset + 6u * groups;
    if (remainder == 2u) {
        const std::int32_t difference = sub_wrap(samples[tail + 1u], previous);
        out[output] = sub_wrap(
            samples[tail], add_wrap(difference / 3, previous));
        out[output + 2u] = difference % 3;
    } else if (remainder == 1u) {
        out[output] = 0;
    } else if (remainder != 0u) {
        return false;
    }
    return true;
}

std::int32_t avg_even_pair(std::int32_t left, std::int32_t right) {
    const std::uint32_t sum = static_cast<std::uint32_t>(left)
        + static_cast<std::uint32_t>(right);
    return codec_v3_arithmetic_shift_right(static_cast<std::int32_t>(sum), 1u);
}

} // namespace

bool compute_mix2_deltas(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    std::vector<std::int32_t>& deltas,
    std::string& error) {
    error.clear();
    deltas.clear();
    // auro::mix::deltas mix2 @ 0x4ED0E0: require equal lengths >= 4.
    if (primary.size() < 4u || primary.size() != secondary.size()) {
        error = "mix2 deltas require matching planes of at least 4 samples";
        return false;
    }
    const std::size_t n = primary.size();
    deltas.assign(n, 0);

    // Odd indices from primary: d[k] = p[k] - avg(even(p[k-1]), even(p[k+1])).
    for (std::size_t k = 1; k + 1u < n; k += 2u) {
        deltas[k] = sub_wrap(primary[k], avg_even_pair(
            mix_even_round(primary[k - 1u]),
            mix_even_round(primary[k + 1u])));
    }
    if ((n & 1u) == 0u)
        deltas[n - 1u] = 0;

    // Even indices from secondary starting at 2.
    for (std::size_t k = 2; k + 1u < n; k += 2u) {
        deltas[k] = sub_wrap(secondary[k], avg_even_pair(
            mix_even_round(secondary[k - 1u]),
            mix_even_round(secondary[k + 1u])));
    }
    if ((n & 1u) != 0u)
        deltas[n - 1u] = 0;

    deltas[0] = 0;
    return true;
}

bool compute_mix3_deltas(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    std::vector<std::int32_t>& deltas,
    std::string& error) {
    error.clear();
    deltas.clear();
    if (primary.size() < 6u || primary.size() != secondary.size()
        || primary.size() != tertiary.size()) {
        error = "mix3 deltas require matching component vectors of length >= 6";
        return false;
    }
    if (primary.size() > std::numeric_limits<std::size_t>::max() / 2u) {
        error = "mix3 delta output size overflows size_t";
        return false;
    }
    deltas.assign(primary.size() * 2u, 0);
    if (!mix3_delta_024(primary, deltas, 2u)
        || !mix3_delta_115(secondary, deltas, 5u)
        || !mix3_delta_224(tertiary, deltas, 7u)) {
        deltas.clear();
        error = "mix3 delta tail shape is not representable";
        return false;
    }
    // ComputeDeltas explicitly clears these four native fixed slots after the
    // three specializations return.
    deltas[0] = 0;
    deltas[1] = 0;
    deltas[3] = 0;
    return true;
}

} // namespace auro3d::encode
