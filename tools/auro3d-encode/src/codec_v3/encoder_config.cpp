#include "encoder_config.hpp"

#include "layout.hpp"

#include <cstddef>
#include <thread>

namespace auro3d::encode {
namespace {

void fill_absent(OptionalU32& field, std::uint32_t present_src, std::uint32_t value_src) {
    if (field.present == 0u && present_src != 0u) {
        field.present = 1u;
        field.value = value_src;
    }
}

void store_clustering_pair(
    DefaultClustering& clustering,
    std::size_t word_offset,
    const OptionalU32& field) {
    clustering.words[word_offset] = field.present;
    clustering.words[word_offset + 1u] = field.value;
}

} // namespace

std::uint32_t encoder_config_init_defaults(EncoderConfig& config) {
    // Direct port of Config::init_defaults @ 0x4F8900.
    std::uint64_t bit_hi = 0xC00000000ull;
    std::uint64_t bit_lo = 3u;
    switch (config.profile) {
    case 0u:
        bit_hi = 0xB00000000ull;
        config.profile = 2u;
        break;
    case 2u:
        bit_hi = 0xB00000000ull;
        break;
    case 3u:
        bit_hi = 0x900000000ull;
        bit_lo = 7u;
        break;
    case 5u:
        bit_hi = 0x800000000ull;
        bit_lo = 8u;
        break;
    default:
        break;
    }

    if (config.bit_line.present == 0u) {
        const std::uint64_t packed = bit_lo | bit_hi;
        config.bit_line.present = 1u;
        config.bit_line.low = static_cast<std::uint32_t>(packed);
        config.bit_line.high = static_cast<std::uint32_t>(packed >> 32u);
    }

    if (config.field_28.present == 0u)
        config.field_28 = {1u, 1u};

    if (config.thread_workers.present == 0u) {
        const unsigned hw = std::thread::hardware_concurrency();
        std::uint8_t workers = 0u;
        if (hw > 0u) {
            const unsigned capped = hw - 1u;
            if (capped < 0x100u)
                workers = static_cast<std::uint8_t>(capped);
        }
        config.thread_workers = {1u, workers};
    }

    if (config.field_44.present == 0u)
        config.field_44 = {1u, 1u};

    // Native stack defaults before the profile table switch.
    std::uint32_t v25[4] = {0u, 0u, 1u, 0u};
    std::uint32_t v26[4] = {0u, 0u, 0u, 0u};
    std::uint32_t v27[4] = {1u, 1u, 0u, 0u};
    std::uint32_t v28[4] = {0u, 0u, 0u, 0u};
    std::uint32_t v29[2] = {0u, 0u};

    std::uint32_t profile_key = config.profile != 0u ? config.profile : 2u;
    switch (profile_key) {
    case 1u:
        // Native remaps pointers; effective writes:
        v25[0] = 1u;
        v25[1] = 0x15Eu;
        v26[0] = 1u;
        v28[0] = 1u;
        v28[1] = 1u;
        v28[2] = 1u;
        v28[3] = 1u;
        v29[0] = 1u;
        v27[2] = 1u;
        if (config.field_52.present == 0u)
            config.field_52 = {1u, v25[1]};
        break;
    case 2u:
        v25[0] = 1u;
        v25[1] = 200u;
        v26[0] = 1u;
        v26[1] = 1u;
        v28[0] = 1u;
        v28[2] = 1u;
        v29[0] = 1u;
        v27[2] = 1u;
        if (config.field_52.present == 0u)
            config.field_52 = {1u, v25[1]};
        break;
    case 3u:
        v25[0] = 1u;
        v25[1] = 0x5Au;
        v26[0] = 1u;
        v26[1] = 1u;
        v28[0] = 1u;
        v28[2] = 1u;
        v29[0] = 1u;
        v29[1] = 1u;
        v27[2] = 1u;
        if (config.field_52.present == 0u)
            config.field_52 = {1u, v25[1]};
        break;
    case 4u:
    case 5u:
        v27[2] = 1u;
        v27[3] = 3u;
        break;
    default:
        break;
    }

    fill_absent(config.field_52, v25[0], v25[1]);
    fill_absent(config.field_60, v25[2], v25[3]);
    fill_absent(config.field_68, v26[0], v26[1]);
    fill_absent(config.field_76, v26[2], v26[3]);
    fill_absent(config.field_84, v27[0], v27[1]);
    fill_absent(config.field_92, v27[2], v27[3]);
    fill_absent(config.field_100, v28[0], v28[1]);
    fill_absent(config.field_108, v28[2], v28[3]);
    fill_absent(config.field_116, v29[0], v29[1]);

    // Native treats +124 as a regular present/value pair, but +132 is read as
    // the downmix-enable value itself (and +136 is its optional marker).
    // Config::init_defaults writes qword 1 at both locations: +124 becomes
    // present=1,value=0, while +132 becomes value=1,present=0.  The latter
    // must remain one because downmix_ gates the complete encode pipeline on
    // this exact dword.
    if (config.field_124.present == 0u)
        config.field_124 = {1u, 0u};
    if (config.field_132.value == 0u) {
        config.field_132.value = 1u;
        config.field_132.present = 0u;
    }
    DefaultClustering clustering{};
    store_clustering_pair(clustering, 0u, config.field_52);
    store_clustering_pair(clustering, 2u, config.field_60);
    store_clustering_pair(clustering, 4u, config.field_68);
    store_clustering_pair(clustering, 6u, config.field_76);
    store_clustering_pair(clustering, 8u, config.field_84);
    store_clustering_pair(clustering, 10u, config.field_92);
    store_clustering_pair(clustering, 12u, config.field_100);
    store_clustering_pair(clustering, 14u, config.field_108);
    store_clustering_pair(clustering, 16u, config.field_116);
    if (!configure_native_cluster_deltas(
            clustering, config.cluster_backend, config.gvm)) {
        return 1u;
    }
    return 0u;
}

std::uint32_t encoder_config_validate(const EncoderConfig& config) {
    // Direct port of Config::validate @ 0x4F8AE0.
    if (config.profile < 1u || config.profile > 5u)
        return 1u;
    if (!codec_v3_unit_block_size_supported(config.unit_block_size))
        return 392u;

    if (config.bit_line.present != 0u) {
        const std::uint32_t low_delta = config.bit_line.low - 3u;
        const std::uint32_t high_delta = config.bit_line.high - 3u;
        if (low_delta > 9u || high_delta > 9u)
            return 1u;
    }

    std::uint32_t carrier = 0u;
    if (!codec_v3_carrier_layout(config.original_layout, carrier))
        return 384u;

    if (!codec_v3_sample_rate_supported(config.sample_rate))
        return 386u;

    if (config.field_52.present != 0u && config.field_52.value == 0u)
        return 1u;

    if (config.field_92.present != 0u && config.field_92.value >= 4u)
        return 1u;
    const NativeClusterDeltasBackend expected_backend =
        config.field_92.present != 0u && config.field_92.value == 3u
        ? NativeClusterDeltasBackend::quantization
        : NativeClusterDeltasBackend::gvm;
    if (config.cluster_backend != expected_backend)
        return 1u;
    if (expected_backend == NativeClusterDeltasBackend::gvm) {
        const std::uint32_t expected_mode =
            config.field_92.present != 0u ? config.field_92.value : 0u;
        const NativeGvmLearnerImplementation expected_learner =
            expected_mode == 2u
            ? NativeGvmLearnerImplementation::old_fast
            : (expected_mode == 1u
                ? NativeGvmLearnerImplementation::unavailable_mode1
                : NativeGvmLearnerImplementation::modern);
        if (config.gvm.mode != expected_mode
            || config.gvm.learner != expected_learner) {
            return 1u;
        }
    } else if (config.gvm.mode != 0u
        || config.gvm.learner != NativeGvmLearnerImplementation::modern
        || config.gvm.has_qword_48
        || config.gvm.flag_40 || config.gvm.flag_41
        || config.gvm.flag_64 || config.gvm.flag_65
        || config.gvm.flag_66 || config.gvm.flag_67) {
        return 1u;
    }
    return 0u;
}

bool encoder_config_prepare(
    EncoderConfig& config,
    std::uint32_t sample_rate,
    std::uint32_t original_layout,
    std::uint32_t unit_block_size,
    std::uint32_t profile,
    std::string& error) {
    error.clear();
    config = {};
    config.sample_rate = sample_rate;
    config.original_layout = original_layout;
    config.unit_block_size = unit_block_size;
    config.profile = profile;
    if (encoder_config_init_defaults(config) != 0u) {
        error = "encoder Config::init_defaults failed";
        return false;
    }
    const std::uint32_t status = encoder_config_validate(config);
    if (status != 0u) {
        error = "encoder Config::validate rejected configuration (status "
            + std::to_string(status) + ")";
        return false;
    }
    return true;
}

bool encoder_config_with_unit_block_size(
    const EncoderConfig& base,
    std::uint32_t unit_block_size,
    EncoderConfig& config,
    std::string& error) {
    error.clear();
    if (!codec_v3_unit_block_size_supported(unit_block_size)) {
        error = "replacement codec-v3 UnitBlock size is unsupported";
        return false;
    }
    config = base;
    config.unit_block_size = unit_block_size;
    const std::uint32_t status = encoder_config_validate(config);
    if (status != 0u) {
        error = "cloned encoder Config::validate rejected configuration (status "
            + std::to_string(status) + ")";
        return false;
    }
    return true;
}

} // namespace auro3d::encode
