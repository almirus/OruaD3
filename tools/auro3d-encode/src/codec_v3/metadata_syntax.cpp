#include "metadata_syntax.hpp"

namespace auro3d::encode {

bool validate_pcm_metadata_prefix(
    const PcmMetadataPrefix& prefix,
    std::string& error) {
    error.clear();
    if (prefix.field_4_0 > 0x0Fu || prefix.field_2_0 > 0x03u
        || prefix.field_4_1 > 0x0Fu) {
        error = "PCM metadata prefix contains an over-width field";
        return false;
    }
    std::size_t present_config_count = 0u;
    for (const std::uint8_t value : prefix.channel_config) {
        if (value != 0xFFu)
            ++present_config_count;
    }
    const std::size_t expected_extension_words = present_config_count == 2u ? 2u
        : present_config_count == 3u ? 5u
        : present_config_count < 2u ? 0u
        : static_cast<std::size_t>(-1);
    if (expected_extension_words == static_cast<std::size_t>(-1)
        || prefix.extension_words.size() != expected_extension_words) {
        error = "PCM metadata prefix extension-word count is invalid";
        return false;
    }
    return true;
}

bool write_pcm_metadata_prefix(PcmMetadataFalseWriter& writer, const PcmMetadataPrefix& prefix) {
    std::string validation_error;
    if (!validate_pcm_metadata_prefix(prefix, validation_error))
        return false;
    if (!writer.write_unsigned(prefix.field_8_0, 8u))
        return false;
    for (const bool value : prefix.flags_1_0) {
        if (!writer.write_bool(value))
            return false;
    }
    if (prefix.field_4_0 > 0x0Fu || !writer.write_unsigned(prefix.field_4_0, 4u))
        return false;
    for (const std::uint8_t value : prefix.fields_8_1) {
        if (!writer.write_unsigned(value, 8u))
            return false;
    }
    for (const bool value : prefix.flags_1_1) {
        if (!writer.write_bool(value))
            return false;
    }
    if (prefix.field_2_0 > 0x03u || prefix.field_4_1 > 0x0Fu
        || !writer.write_unsigned(prefix.field_2_0, 2u)
        || !writer.write_unsigned(prefix.field_4_1, 4u)
        || !writer.write_unsigned(prefix.field_8_2, 8u)
        || !writer.write_unsigned(prefix.adol_block_count, 8u)
        || !writer.write_unsigned(prefix.field_8_3, 8u)) {
        return false;
    }
    for (const std::uint8_t value : prefix.channel_config) {
        if (!writer.write_unsigned(value, 8u))
            return false;
    }
    for (const std::uint8_t value : prefix.reserved) {
        if (!writer.write_unsigned(value, 8u))
            return false;
    }
    for (const std::uint32_t value : prefix.extension_words) {
        if (!writer.write_unsigned(value, 32u))
            return false;
    }
    return true;
}

} // namespace auro3d:encode
