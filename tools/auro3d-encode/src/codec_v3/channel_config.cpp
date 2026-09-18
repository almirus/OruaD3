#include "channel_config.hpp"

namespace auro3d::encode {
namespace {

struct ChannelConfig {
    std::uint8_t id;
    std::uint32_t original_layout;
    std::uint32_t carrier_layout;
};

// auro_adol_channel_input_config_get_original_layout and
// auro_adol_channel_input_config_get_carrier_layout from decoder.cpp. Only
// their direct encoder-safe IDs are represented here.
constexpr ChannelConfig kChannelConfigs[] = {
    {1u, 55u, 3u}, {2u, 63u, 11u}, {8u, 71u, 3u}, {11u, 1587u, 3u},
    {12u, 51u, 3u}, {15u, 1599u, 63u}, {20u, 26163u, 51u},
    {30u, 26175u, 63u}, {40u, 30271u, 63u}, {50u, 32319u, 63u},
    {54u, 26559u, 447u}, {62u, 32703u, 447u}, {64u, 3u, 4u},
    {66u, 7u, 3u}, {67u, 119u, 55u}, {68u, 127u, 63u},
    {69u, 439u, 55u}, {70u, 447u, 63u}, {71u, 26167u, 55u},
    {72u, 30263u, 55u}, {73u, 32311u, 55u}, {74u, 26551u, 439u},
    {75u, 1983u, 447u}, {76u, 30647u, 439u}, {77u, 30655u, 447u},
    {78u, 32695u, 439u}, {128u, 4u, 4u}, {129u, 2052u, 4u},
    {130u, 6148u, 4u},
};

} // namespace

bool codec_v3_channel_config_to_layout(
    std::uint8_t config_id,
    std::uint32_t& original_layout) {
    for (const ChannelConfig& config : kChannelConfigs) {
        if (config.id == config_id) {
            original_layout = config.original_layout;
            return true;
        }
    }
    return false;
}

bool codec_v3_layout_to_channel_config(
    std::uint32_t original_layout,
    std::uint8_t& config_id) {
    for (const ChannelConfig& config : kChannelConfigs) {
        if (config.original_layout == original_layout) {
            config_id = config.id;
            return true;
        }
    }
    return false;
}

bool codec_v3_channel_config_to_carrier_layout(
    std::uint8_t config_id,
    std::uint32_t& carrier_layout) {
    for (const ChannelConfig& config : kChannelConfigs) {
        if (config.id == config_id) {
            carrier_layout = config.carrier_layout;
            return true;
        }
    }
    return false;
}

} // namespace auro3d:encode
