#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d {

// Pointer-free AM4HP XinN input presets extracted from the matching native
// image. The records are kept as bytes because CoreProcessor's native
// construction path consumes the original packed layout, not a C++ model.
class Am4hpInputPresets {
public:
    static constexpr std::size_t kRecordCount = 2u;
    static constexpr std::size_t kRecordBytes = 23856u;
    static constexpr std::uint32_t kStereoInputMask = 3u;
    static constexpr std::uint32_t kSurroundInputMask = 51u;

    bool load_embedded(std::string& error);
    bool loaded() const noexcept { return bytes_.size() == kRecordCount * kRecordBytes; }

    const std::uint8_t* record(std::size_t index) const noexcept;
    // Native AM4HP Upmixing selects the 2.0 record for mask 3 and the 5.1
    // record for mask 0x33; all other input masks are rejected by this owner.
    const std::uint8_t* record_for_layout(std::uint32_t layout_mask) const noexcept;
    std::size_t record_size() const noexcept { return kRecordBytes; }

    // Copy a record into caller-owned storage and restore its six internal
    // table pointers so the copy can be used as stable XinN backing.
    bool copy_retargeted_record(std::uint32_t layout_mask,
                                std::vector<std::uint8_t>& output,
                                std::string& error) const;

private:
    std::vector<std::uint8_t> bytes_;
};

}
