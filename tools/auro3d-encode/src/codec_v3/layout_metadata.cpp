#include "layout_metadata.hpp"

#include "channel_config.hpp"
#include "layout.hpp"
#include "scaler.hpp"

#include <cmath>

namespace auro3d::encode {
namespace {

constexpr std::array<std::uint8_t, 6> kCompactSecondaryGainShifts{
    0u, 8u, 4u, 20u, 12u, 16u,
};

constexpr std::array<std::uint8_t, 9> kSevenOneSecondaryGainShifts{
    0u, 8u, 4u, 28u, 12u, 16u, 0u, 20u, 24u,
};

bool secondary_gain_shift(
    std::uint32_t carrier_layout,
    std::uint32_t channel,
    std::uint8_t& shift) {
    constexpr std::uint64_t kCompactCarrierLayouts = 0x8088000000000818ull;
    if (carrier_layout <= 0x3Fu
        && ((kCompactCarrierLayouts >> carrier_layout) & 1u) != 0u) {
        if (channel >= kCompactSecondaryGainShifts.size())
            return false;
        shift = kCompactSecondaryGainShifts[channel];
        return true;
    }
    if ((carrier_layout & 0xFFFFFFF7u) == 0x1B7u) {
        constexpr std::uint32_t kSevenOneGainChannels = 447u;
        if (channel >= kSevenOneSecondaryGainShifts.size()
            || (kSevenOneGainChannels & (std::uint32_t{1} << channel)) == 0u) {
            return false;
        }
        shift = kSevenOneSecondaryGainShifts[channel];
        return true;
    }
    return false;
}

std::int64_t round_half_away_from_zero(double value) {
    return value < 0.0
        ? -static_cast<std::int64_t>(std::floor(-value + 0.5))
        : static_cast<std::int64_t>(std::floor(value + 0.5));
}

bool quantize_loudness_value(
    std::uint32_t type,
    double value,
    std::uint32_t& quantized) {
    if (!std::isfinite(value) || type > 5u)
        return false;
    const std::int64_t rounded =
        round_half_away_from_zero(value * 10.0);
    std::int64_t biased = rounded;
    std::int64_t maximum = 0;
    switch (type) {
    case 0u:
    case 2u:
        biased += 350;
        maximum = 330;
        break;
    case 1u:
        maximum = 300;
        break;
    case 3u:
        biased += 10;
        maximum = 50;
        break;
    case 4u:
    case 5u:
        biased += 1000;
        maximum = 1000;
        break;
    default:
        return false;
    }
    if (biased < 0 || biased > maximum)
        return false;
    quantized = static_cast<std::uint32_t>(biased);
    return true;
}

} // namespace

bool make_layout_adol_block(
    std::uint32_t original_layout,
    std::vector<AdolInstruction>& instructions,
    std::string& error) {
    error.clear();
    instructions.clear();
    std::uint8_t config_id = 0;
    std::uint32_t config_carrier = 0;
    std::uint32_t expected_carrier = 0;
    if (!codec_v3_layout_to_channel_config(original_layout, config_id) ||
        !codec_v3_channel_config_to_carrier_layout(config_id, config_carrier) ||
        !codec_v3_carrier_layout(original_layout, expected_carrier) ||
        config_carrier != expected_carrier) {
        error = "layout has no direct codec-v3 ADOL channel-input configuration";
        return false;
    }
    instructions.push_back({0x1Eu, config_id, 0u});
    return true;
}

bool append_primary_downmix_gain_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t channel,
    std::uint32_t scaler_index,
    std::string& error) {
    error.clear();
    if (channel > 30u || scaler_index > 0xFFu
        || validate_scaler_index(
            static_cast<std::uint8_t>(scaler_index))
            != scaler_index) {
        error = "primary downmix ADOL fields fail native validation";
        return false;
    }
    instructions.push_back({0x40u, channel, scaler_index});
    return true;
}

bool pack_secondary_downmix_gains(
    std::uint32_t carrier_layout,
    const std::array<float, 31>& gains_db,
    std::uint32_t& packed,
    std::string& error) {
    error.clear();
    packed = 0u;
    std::vector<std::uint32_t> channels;
    if (!codec_v3_layout_channel_order(carrier_layout, channels)) {
        error = "secondary downmix carrier layout is empty";
        return false;
    }
    for (const std::uint32_t channel : channels) {
        std::uint8_t shift = 0u;
        if (!secondary_gain_shift(carrier_layout, channel, shift)) {
            error = "secondary downmix gains do not support this carrier layout";
            return false;
        }
        const float gain = gains_db[channel];
        if (!std::isfinite(gain) || gain > 0.0f) {
            error = "secondary downmix gain must be finite and non-positive";
            return false;
        }

        // from_secondary_downmix_gains @ 0x4FF600:
        //   q = trunc(gain * -100 + 75)
        //   nibble = q <= 2399 ? q / 150 : 15
        const float biased = gain * -100.0f + 75.0f;
        const std::uint32_t quantized = biased >= 2400.0f
            ? 15u
            : static_cast<std::uint32_t>(biased) / 150u;
        packed |= quantized << shift;
    }
    return true;
}

bool append_secondary_downmix_gains_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t carrier_layout,
    const std::array<float, 31>& gains_db,
    std::string& error) {
    std::uint32_t packed = 0u;
    if (!pack_secondary_downmix_gains(
            carrier_layout, gains_db, packed, error)) {
        return false;
    }
    instructions.push_back({0x46u, packed, 0u});
    return true;
}

bool pack_loudness_metadata(
    const LoudnessMetadata& loudness,
    std::uint32_t& packed,
    std::string& error) {
    error.clear();
    packed = 0u;
    if (loudness.type > 5u) {
        error = "loudness metadata type is outside native range";
        return false;
    }
    std::uint32_t value0 = 0u;
    std::uint32_t value1 = 0u;
    std::uint32_t value2 = 0u;
    if (!quantize_loudness_value(loudness.type, loudness.value, value0)
        || (loudness.value_2_present
            && !quantize_loudness_value(
                loudness.type, loudness.value_2, value1))
        || (loudness.value_3_present
            && !quantize_loudness_value(
                loudness.type, loudness.value_3, value2))) {
        error = "loudness value is outside the native range for its type";
        return false;
    }
    packed = (value0 << 22u)
        | (loudness.value_2_present ? (std::uint32_t{1} << 21u) : 0u)
        | (value1 << 11u)
        | (loudness.value_3_present ? (std::uint32_t{1} << 10u) : 0u)
        | value2;
    return true;
}

bool append_loudness_adol(
    std::vector<AdolInstruction>& instructions,
    const LoudnessMetadata& loudness,
    std::string& error) {
    static constexpr std::array<std::uint8_t, 6> kLoudnessOpcodes{
        0x80u, 0x81u, 0x85u, 0x82u, 0x83u, 0x84u,
    };
    std::uint32_t packed = 0u;
    if (!pack_loudness_metadata(loudness, packed, error))
        return false;
    instructions.push_back({
        kLoudnessOpcodes[loudness.type],
        packed,
        0u,
    });
    return true;
}

bool append_limit_simple_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t scaler_index,
    std::string& error) {
    error.clear();
    if (scaler_index > 0xFFu
        || validate_scaler_index(
            static_cast<std::uint8_t>(scaler_index))
            != scaler_index) {
        error = "limit-simple ADOL scaler fails native validation";
        return false;
    }
    instructions.push_back({0x41u, scaler_index, 0u});
    return true;
}

bool append_auromatic_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t profile,
    std::uint32_t mode,
    std::string& error) {
    error.clear();
    if (profile >= 0x10u || mode >= 0x10u) {
        error = "auromatic ADOL profile/mode must be below 16";
        return false;
    }
    // from_auromatic @ 0x515020: DWORD1 = (profile << 4) | mode.
    instructions.push_back({0x47u, (profile << 4u) | mode, 0u});
    return true;
}

bool append_encoder_version_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t version,
    std::string& error) {
    error.clear();
    if ((version & ~0x00FFFFFFu) != 0u) {
        error = "encoder-version ADOL value exceeds 24 bits";
        return false;
    }
    instructions.push_back({0x64u, version, 0u});
    return true;
}

bool append_opcode_50_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t value,
    std::string& error) {
    error.clear();
    if (value > 0xFFu) {
        error = "ADOL opcode 0x50 payload exceeds 8 bits";
        return false;
    }
    instructions.push_back({0x50u, value, 0u});
    return true;
}

bool append_opcode_6e_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t value,
    std::string& error) {
    error.clear();
    static_cast<void>(error);
    instructions.push_back({0x6Eu, value, 0u});
    return true;
}

} // namespace auro3d::encode
