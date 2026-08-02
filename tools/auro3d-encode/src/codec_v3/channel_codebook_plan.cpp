#include "channel_codebook_plan.hpp"

#include "channel_metadata.hpp"
#include "channel_payload.hpp"
#include "error_quantization.hpp"

#include <limits>

namespace auro3d::encode {

bool make_channel_codebook_prefix(
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t golomb_parameter,
    std::uint32_t& selector,
    std::array<std::uint32_t, 3>& metadata_words,
    std::vector<std::uint32_t>& context_words,
    std::string& error) {
    error.clear();
    selector = 0u;
    metadata_words = {};
    context_words.clear();
    if (mode != 2u && mode != 3u) {
        error = "channel codebook prefix requires mode 2 or 3";
        return false;
    }
    if (bit_width == 0u || bit_width > 32u) {
        error = "channel codebook bit width is outside the native range";
        return false;
    }
    if (golomb_parameter > 15u) {
        error = "channel codebook Golomb-Rice parameter is outside metadata range";
        return false;
    }
    if (values0.empty()
        || (mode == 2u && !values1.empty())
        || (mode == 3u && values1.size() != values0.size())) {
        error = "channel codebook dimensions or size are invalid";
        return false;
    }
    if (values0.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "channel codebook has too many entries";
        return false;
    }
    if (values0.size() > kCodecV3ChannelCodebookMaxEntries) {
        error = "channel codebook exceeds native selector capacity";
        return false;
    }
    std::uint32_t base_index = 0u;
    if (!channel_metadata_selector_from_count(
            static_cast<std::uint32_t>(values0.size()), selector, base_index)) {
        error = "channel codebook size has no native metadata selector";
        return false;
    }
    // Native channel metadata selects a capacity, not the exact number of
    // residual entries.  The context stream therefore contains zero-filled
    // tail entries up to that selected capacity.
    std::vector<std::int32_t> padded0 = values0;
    padded0.resize(base_index, 0);
    std::vector<std::uint32_t> encoded0;
    if (!pack_golomb_errors(padded0, bit_width, encoded0, error))
        return false;
    if (mode == 2u) {
        if (!pack_channel_context_values(encoded0, bit_width, context_words, error))
            return false;
    } else {
        std::vector<std::int32_t> padded1 = values1;
        padded1.resize(base_index, 0);
        std::vector<std::uint32_t> encoded1;
        if (!pack_golomb_errors(padded1, bit_width, encoded1, error))
            return false;
        std::vector<std::uint32_t> combined;
        if (encoded0.size() > std::numeric_limits<std::size_t>::max() - encoded1.size()) {
            error = "channel codebook context size overflows storage";
            return false;
        }
        combined.reserve(encoded0.size() + encoded1.size());
        combined.insert(combined.end(), encoded0.begin(), encoded0.end());
        combined.insert(combined.end(), encoded1.begin(), encoded1.end());
        if (!pack_channel_context_values(combined, bit_width, context_words, error))
            return false;
    }
    if (!make_channel_metadata_words(
            bit_width, selector, channel_ids, third_word, metadata_words)) {
        error = "channel codebook metadata words are invalid";
        return false;
    }
    metadata_words[0] |= golomb_parameter << 24u;
    ChannelMetadataCombined combined{};
    if (!combine_channel_metadata(0u, metadata_words, combined)
        || combined.mode != mode || combined.bit_width != bit_width
        || combined.base_index != base_index) {
        error = "channel codebook metadata does not describe its context";
        return false;
    }
    return true;
}

} // namespace auro3d::encode
