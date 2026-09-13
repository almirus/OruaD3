#include "am4hp_input_presets.hpp"

#define NOMINMAX
#include <windows.h>

#include <cstring>

namespace auro3d {

const std::uint8_t* Am4hpInputPresets::record(std::size_t index) const noexcept {
    if (!loaded() || index >= kRecordCount)
        return nullptr;
    return bytes_.data() + index * kRecordBytes;
}

const std::uint8_t* Am4hpInputPresets::record_for_layout(
    std::uint32_t layout_mask) const noexcept {
    if (layout_mask == kStereoInputMask)
        return record(0u);
    if (layout_mask == kSurroundInputMask)
        return record(1u);
    return nullptr;
}

bool Am4hpInputPresets::copy_retargeted_record(
    std::uint32_t layout_mask,
    std::vector<std::uint8_t>& output,
    std::string& error) const {
    const auto* source = record_for_layout(layout_mask);
    if (!source) {
        error = "unsupported AM4HP XinN input layout";
        return false;
    }
    std::vector<std::uint8_t> candidate(source, source + kRecordBytes);
    constexpr std::size_t kEngine1Tables[3] = {11760u, 15240u, 18712u};
    // Native 0x54E2xx stores one distinct 136-byte Engine2 table per bus.
    // The extracted resource keeps those tables consecutively after the
    // pointer array, so each native pointer must retain its bus-specific
    // offset rather than collapsing all three buses onto bus 0.
    constexpr std::size_t kEngine2TableBase = 23328u;
    constexpr std::size_t kEngine2TableBytes = 136u;
    auto write_pointer = [&](std::size_t field, std::size_t target) {
        const auto address = reinterpret_cast<std::uintptr_t>(
            candidate.data() + target);
        std::memcpy(candidate.data() + field, &address, sizeof(address));
    };
    for (std::size_t table = 0u; table < 3u; ++table)
        write_pointer(48u + 8u * table, kEngine1Tables[table]);
    for (std::size_t bus = 0u; bus < 3u; ++bus)
        write_pointer(1048u + 8u * bus,
                      kEngine2TableBase + kEngine2TableBytes * bus);
    output = std::move(candidate);
    return true;
}

bool Am4hpInputPresets::load_embedded(std::string& error) {
    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(105),
                                   MAKEINTRESOURCEW(10));
    if (!resource) {
        error = "embedded AM4HP input-preset resource not found";
        return false;
    }
    HGLOBAL loaded = LoadResource(nullptr, resource);
    const auto* data = static_cast<const std::uint8_t*>(
        loaded ? LockResource(loaded) : nullptr);
    const DWORD size = SizeofResource(nullptr, resource);
    if (!data || size != kRecordCount * kRecordBytes) {
        error = "invalid embedded AM4HP input-preset resource size";
        return false;
    }

    std::vector<std::uint8_t> candidate(data, data + size);
    for (std::size_t index = 0u; index < kRecordCount; ++index) {
        const auto* record = candidate.data() + index * kRecordBytes;
        if (record[0] != 1u || record[1] != 0xA5u) {
            error = "invalid AM4HP input-preset record marker";
            return false;
        }
        // The extractor deliberately removes six native absolute pointers.
        // Rejecting a nonzero field here prevents accidental use of a record
        // tied to a different address space or library build.
        for (std::size_t table = 0u; table < 3u; ++table) {
            const auto* field = record + 8u + 40u + 8u * table;
            for (std::size_t byte = 0u; byte < 8u; ++byte)
                if (field[byte] != 0u) {
                    error = "AM4HP input-preset record contains native pointer";
                    return false;
                }
        }
        for (std::size_t bus = 0u; bus < 3u; ++bus) {
            const auto* field = record + 8u + 1040u + 8u * bus;
            for (std::size_t byte = 0u; byte < 8u; ++byte)
                if (field[byte] != 0u) {
                    error = "AM4HP input-preset record contains native pointer";
                    return false;
                }
        }
    }
    bytes_ = std::move(candidate);
    return true;
}

}
