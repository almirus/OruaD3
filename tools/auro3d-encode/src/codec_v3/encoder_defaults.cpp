#include "encoder_defaults.hpp"

namespace auro3d::encode {

bool codec_v3_default_bit_line(
    std::uint32_t profile,
    std::uint8_t parameter,
    DefaultBitLine& out) {
    out = {};
    std::uint64_t value = 0;
    switch (profile) {
    case 2u:
        value = (static_cast<std::uint64_t>(parameter) + 3u) | 0xB00000000ull;
        break;
    case 3u:
        value = (static_cast<std::uint64_t>((parameter ^ 1u) | 8u) << 32u)
            | static_cast<std::uint64_t>(parameter ^ 7u);
        break;
    case 4u:
        value = (parameter != 0u ? 0x600000000ull : 0xC00000000ull)
            | (3ull * parameter + 3u);
        break;
    case 5u:
        value = (static_cast<std::uint64_t>(parameter ^ 1u) << 33u)
            + 0x600000000ull | (2ull * (parameter ^ 1u) + 6u);
        break;
    default:
        value = 0xC00000003ull;
        break;
    }
    out.low = static_cast<std::uint32_t>(value);
    out.high = static_cast<std::uint32_t>(value >> 32u);
    return profile >= 2u && profile <= 5u;
}

bool codec_v3_bit_line_quality(std::uint32_t bit_line, std::uint32_t& quality) {
    // Native Encoder:add_: if (bit_line - 3 <= 0xB) use [bit_line - 3], else 80.
    static constexpr std::uint32_t kTable[12] = {
        15u, 20u, 40u, 60u, 80u, 100u, 120u, 140u, 160u, 180u, 200u, 220u,
    };
    if (bit_line >= 3u && bit_line - 3u <= 0xBu) {
        quality = kTable[bit_line - 3u];
        return true;
    }
    quality = 80u;
    return false;
}

bool codec_v3_default_clustering(
    std::uint32_t profile,
    DefaultClustering& out) {
    out = {};
    // default_clustering initializes +8, +32, and +36 before selecting the
    // profile-specific table. The writes below preserve the exact native
    // byte offsets (all values are dwords) without inventing field names.
    out.words[2] = 1u;
    out.words[8] = 1u;
    out.words[9] = 1u;
    const std::uint32_t selected = profile != 0u ? profile : 2u;
    switch (selected) {
    case 1u:
        out.words[0] = 1u;
        out.words[1] = 0x15Eu;
        out.words[4] = 1u;
        out.words[5] = 1u;
        out.words[10] = 1u;
        out.words[12] = 1u;
        out.words[13] = 1u;
        out.words[14] = 1u;
        out.words[15] = 1u;
        out.words[16] = 1u;
        return true;
    case 2u:
        out.words[0] = 1u;
        out.words[1] = 200u;
        out.words[4] = 1u;
        out.words[5] = 1u;
        out.words[10] = 1u;
        out.words[12] = 1u;
        out.words[14] = 1u;
        out.words[16] = 1u;
        return true;
    case 3u:
        out.words[0] = 1u;
        out.words[1] = 0x5Au;
        out.words[4] = 1u;
        out.words[5] = 1u;
        out.words[10] = 1u;
        out.words[12] = 1u;
        out.words[14] = 1u;
        out.words[15] = 1u;
        out.words[17] = 1u;
        return true;
    case 4u:
    case 5u:
        out.words[10] = 1u;
        out.words[11] = 3u;
        return true;
    default:
        return false;
    }
}

bool configure_native_gvm(
    const DefaultClustering& defaults,
    NativeGvmConfiguration& out) {
    out = {};
    // GVM:configure initializes dword +64 to 65793 (0x00010101)
    // before applying the optional pairs.
    out.flag_64 = true;
    out.flag_65 = true;
    out.flag_66 = true;
    const auto& words = defaults.words;
    out.flag_40 = words[2] != 0u && words[3] != 0u;
    out.flag_41 = words[4] != 0u && words[5] != 0u;
    if (words[6] != 0u) {
        out.has_qword_48 = true;
        out.qword_48 = words[7];
    }
    if (words[8] != 0u)
        out.flag_64 = words[9] != 0u;
    if (words[12] != 0u)
        out.flag_65 = words[13] != 0u;
    if (words[14] != 0u)
        out.flag_66 = words[15] != 0u;
    if (words[16] != 0u)
        out.flag_67 = words[17] != 0u;
    if (words[10] != 0u) {
        if (words[11] > 2u)
            return false;
        out.mode = words[11];
    }
    if (out.mode == 2u) {
        out.learner = NativeGvmLearnerImplementation::old_fast;
    } else if (out.mode == 1u) {
        out.learner = NativeGvmLearnerImplementation::unavailable_mode1;
    } else {
        out.learner = NativeGvmLearnerImplementation::modern;
    }
    return true;
}

bool configure_native_cluster_deltas(
    const DefaultClustering& defaults,
    NativeClusterDeltasBackend& backend,
    NativeGvmConfiguration& gvm) {
    const auto& words = defaults.words;
    if (words[10] != 0u && words[11] == 3u) {
        backend = NativeClusterDeltasBackend::quantization;
        gvm = {};
        return true;
    }
    backend = NativeClusterDeltasBackend::gvm;
    return configure_native_gvm(defaults, gvm);
}

} // namespace auro3d:encode
