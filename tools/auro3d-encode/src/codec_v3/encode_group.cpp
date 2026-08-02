#include "encode_group.hpp"

namespace auro3d::encode {

bool attach_group_channel_pcm(
    EncodeGroup& group,
    std::uint32_t channel_id,
    const std::int32_t* begin,
    const std::int32_t* end,
    std::string& error) {
    error.clear();
    // sub_4E83F0 @ 0x4E83F0: require a non-null closed-open range with begin != end.
    if (begin == nullptr || end == nullptr || begin == end || end < begin) {
        error = "group channel PCM range is empty or invalid";
        return false;
    }
    EncodeGroupFrame frame{};
    frame.channel_id = channel_id;
    frame.samples.assign(begin, end);
    group.frames.push_back(std::move(frame));
    return true;
}

} // namespace auro3d::encode
