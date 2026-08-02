#include "original_channels.hpp"

namespace auro3d::encode {
namespace {

constexpr std::uint32_t kErrUnknownLayout = 407u;
constexpr std::uint32_t kErrUnknownChannel = 408u;

void set_arity1(OriginalChannelGroup& out, std::uint32_t ch0) {
    out.arity = 1u;
    out.channels = {ch0, 0u, 0u};
}

void set_arity2(OriginalChannelGroup& out, std::uint64_t packed) {
    out.arity = 2u;
    out.channels = {
        static_cast<std::uint32_t>(packed),
        static_cast<std::uint32_t>(packed >> 32u),
        0u};
}

void set_arity3(OriginalChannelGroup& out, std::uint64_t packed_low, std::uint32_t ch2) {
    out.arity = 3u;
    out.channels = {
        static_cast<std::uint32_t>(packed_low),
        static_cast<std::uint32_t>(packed_low >> 32u),
        ch2};
}

} // namespace

std::uint32_t get_original_channels(
    std::uint32_t original_layout,
    std::uint32_t carrier_channel,
    OriginalChannelGroup& out) {
    out = {};
    // Direct structural port of auro::codec::v3::get_original_channels @ 0x50F8E0.
    if (original_layout > 2051u) {
        if (original_layout > 30262u) {
            if (original_layout > 32310u) {
                if (original_layout > 32694u) {
                    if (original_layout == 32695u) {
                        switch (carrier_channel) {
                        case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                        case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                        case 2u: set_arity2(out, 0xB00000002ull); return 0u;
                        case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                        case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                        case 7u: set_arity1(out, 7u); return 0u;
                        case 8u: set_arity1(out, 8u); return 0u;
                        default: return kErrUnknownChannel;
                        }
                    }
                    if (original_layout == 32703u) {
                        switch (carrier_channel) {
                        case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                        case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                        case 2u: set_arity2(out, 0xB00000002ull); return 0u;
                        case 3u: set_arity1(out, 3u); return 0u;
                        case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                        case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                        case 7u: set_arity1(out, 7u); return 0u;
                        case 8u: set_arity1(out, 8u); return 0u;
                        default: return kErrUnknownChannel;
                        }
                    }
                    return kErrUnknownLayout;
                }
                if (original_layout == 32311u) {
                    switch (carrier_channel) {
                    case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                    case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                    case 2u: set_arity2(out, 0xB00000002ull); return 0u;
                    case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                    case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                    default: return kErrUnknownChannel;
                    }
                }
                if (original_layout == 32319u) {
                    switch (carrier_channel) {
                    case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                    case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                    case 2u: set_arity2(out, 0xB00000002ull); return 0u;
                    case 3u: set_arity1(out, 3u); return 0u;
                    case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                    case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                    default: return kErrUnknownChannel;
                    }
                }
                return kErrUnknownLayout;
            }
            if (original_layout > 30646u) {
                if (original_layout == 30647u) {
                    switch (carrier_channel) {
                    case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                    case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                    case 2u: set_arity1(out, 2u); return 0u;
                    case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                    case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                    case 7u: set_arity1(out, 7u); return 0u;
                    case 8u: set_arity1(out, 8u); return 0u;
                    default: return kErrUnknownChannel;
                    }
                }
                if (original_layout == 30655u) {
                    switch (carrier_channel) {
                    case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                    case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                    case 2u: set_arity1(out, 2u); return 0u;
                    case 3u: set_arity1(out, 3u); return 0u;
                    case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                    case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                    case 7u: set_arity1(out, 7u); return 0u;
                    case 8u: set_arity1(out, 8u); return 0u;
                    default: return kErrUnknownChannel;
                    }
                }
                return kErrUnknownLayout;
            }
            if (original_layout == 30263u) {
                switch (carrier_channel) {
                case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                case 2u: set_arity1(out, 2u); return 0u;
                case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                default: return kErrUnknownChannel;
                }
            }
            if (original_layout == 30271u) {
                switch (carrier_channel) {
                case 0u: set_arity3(out, 0x900000000ull, 12u); return 0u;
                case 1u: set_arity3(out, 0xA00000001ull, 12u); return 0u;
                case 2u: set_arity1(out, 2u); return 0u;
                case 3u: set_arity1(out, 3u); return 0u;
                case 4u: set_arity3(out, 0xD00000004ull, 12u); return 0u;
                case 5u: set_arity3(out, 0xE00000005ull, 12u); return 0u;
                default: return kErrUnknownChannel;
                }
            }
            return kErrUnknownLayout;
        }
        if (original_layout > 26166u) {
            if (original_layout > 26550u) {
                if (original_layout == 26551u) {
                    switch (carrier_channel) {
                    case 0u: set_arity2(out, 0x900000000ull); return 0u;
                    case 1u: set_arity2(out, 0xA00000001ull); return 0u;
                    case 2u: set_arity1(out, 2u); return 0u;
                    case 4u: set_arity2(out, 0xD00000004ull); return 0u;
                    case 5u: set_arity2(out, 0xE00000005ull); return 0u;
                    case 7u: set_arity1(out, 7u); return 0u;
                    case 8u: set_arity1(out, 8u); return 0u;
                    default: return kErrUnknownChannel;
                    }
                }
                if (original_layout == 26559u) {
                    switch (carrier_channel) {
                    case 0u: set_arity2(out, 0x900000000ull); return 0u;
                    case 1u: set_arity2(out, 0xA00000001ull); return 0u;
                    case 2u: set_arity1(out, 2u); return 0u;
                    case 3u: set_arity1(out, 3u); return 0u;
                    case 4u: set_arity2(out, 0xD00000004ull); return 0u;
                    case 5u: set_arity2(out, 0xE00000005ull); return 0u;
                    case 7u: set_arity1(out, 7u); return 0u;
                    case 8u: set_arity1(out, 8u); return 0u;
                    default: return kErrUnknownChannel;
                    }
                }
                return kErrUnknownLayout;
            }
            if (original_layout == 26167u) {
                switch (carrier_channel) {
                case 0u: set_arity2(out, 0x900000000ull); return 0u;
                case 1u: set_arity2(out, 0xA00000001ull); return 0u;
                case 2u: set_arity1(out, 2u); return 0u;
                case 4u: set_arity2(out, 0xD00000004ull); return 0u;
                case 5u: set_arity2(out, 0xE00000005ull); return 0u;
                default: return kErrUnknownChannel;
                }
            }
            if (original_layout == 26175u) {
                switch (carrier_channel) {
                case 0u: set_arity2(out, 0x900000000ull); return 0u;
                case 1u: set_arity2(out, 0xA00000001ull); return 0u;
                case 2u: set_arity1(out, 2u); return 0u;
                case 3u: set_arity1(out, 3u); return 0u;
                case 4u: set_arity2(out, 0xD00000004ull); return 0u;
                case 5u: set_arity2(out, 0xE00000005ull); return 0u;
                default: return kErrUnknownChannel;
                }
            }
            return kErrUnknownLayout;
        }
        switch (original_layout) {
        case 2052u:
            if (carrier_channel != 2u)
                return kErrUnknownChannel;
            set_arity2(out, 0xB00000002ull);
            return 0u;
        case 6148u:
            if (carrier_channel != 2u)
                return kErrUnknownChannel;
            set_arity3(out, 0xB00000002ull, 12u);
            return 0u;
        case 26163u:
            switch (carrier_channel) {
            case 0u: set_arity2(out, 0x900000000ull); return 0u;
            case 1u: set_arity2(out, 0xA00000001ull); return 0u;
            case 4u: set_arity2(out, 0xD00000004ull); return 0u;
            case 5u: set_arity2(out, 0xE00000005ull); return 0u;
            default: return kErrUnknownChannel;
            }
        default:
            return kErrUnknownLayout;
        }
    }

    if (original_layout <= 118u) {
        switch (original_layout) {
        case 3u:
            if (carrier_channel != 2u)
                return kErrUnknownChannel;
            set_arity2(out, 0x100000000ull);
            return 0u;
        case 4u:
            if (carrier_channel != 2u)
                return kErrUnknownChannel;
            set_arity1(out, 2u);
            return 0u;
        case 7u:
            if (carrier_channel == 1u) {
                set_arity2(out, 0x200000001ull);
                return 0u;
            }
            if (carrier_channel == 0u) {
                set_arity2(out, 0x200000000ull);
                return 0u;
            }
            return kErrUnknownChannel;
        case 51u:
            if (carrier_channel == 1u) {
                set_arity2(out, 0x500000001ull);
                return 0u;
            }
            if (carrier_channel == 0u) {
                set_arity2(out, 0x400000000ull);
                return 0u;
            }
            return kErrUnknownChannel;
        case 55u:
        case 63u:
            if (original_layout == 63u && carrier_channel == 3u) {
                set_arity1(out, 3u);
                return 0u;
            }
            if (carrier_channel == 1u) {
                set_arity3(out, 0x200000001ull, 5u);
                return 0u;
            }
            if (carrier_channel == 0u) {
                set_arity3(out, 0x200000000ull, 4u);
                return 0u;
            }
            return kErrUnknownChannel;
        case 71u:
            if (carrier_channel == 1u) {
                set_arity3(out, 0x200000001ull, 6u);
                return 0u;
            }
            if (carrier_channel == 0u) {
                set_arity3(out, 0x200000000ull, 6u);
                return 0u;
            }
            return kErrUnknownChannel;
        default:
            return kErrUnknownLayout;
        }
    }

    if (original_layout <= 446u) {
        switch (original_layout) {
        case 119u:
            switch (carrier_channel) {
            case 0u: set_arity1(out, 0u); return 0u;
            case 1u: set_arity1(out, 1u); return 0u;
            case 2u: set_arity1(out, 2u); return 0u;
            case 4u: set_arity2(out, 0x600000004ull); return 0u;
            case 5u: set_arity2(out, 0x600000005ull); return 0u;
            default: return kErrUnknownChannel;
            }
        case 127u:
            switch (carrier_channel) {
            case 0u: set_arity1(out, 0u); return 0u;
            case 1u: set_arity1(out, 1u); return 0u;
            case 2u: set_arity1(out, 2u); return 0u;
            case 3u: set_arity1(out, 3u); return 0u;
            case 4u: set_arity2(out, 0x600000004ull); return 0u;
            case 5u: set_arity2(out, 0x600000005ull); return 0u;
            default: return kErrUnknownChannel;
            }
        case 439u:
            switch (carrier_channel) {
            case 0u: set_arity1(out, 0u); return 0u;
            case 1u: set_arity1(out, 1u); return 0u;
            case 2u: set_arity1(out, 2u); return 0u;
            case 4u: set_arity2(out, 0x700000004ull); return 0u;
            case 5u: set_arity2(out, 0x800000005ull); return 0u;
            default: return kErrUnknownChannel;
            }
        default:
            return kErrUnknownLayout;
        }
    }

    if (original_layout > 1598u) {
        if (original_layout == 1599u) {
            switch (carrier_channel) {
            case 0u: set_arity2(out, 0x900000000ull); return 0u;
            case 1u: set_arity2(out, 0xA00000001ull); return 0u;
            case 2u: set_arity1(out, 2u); return 0u;
            case 3u: set_arity1(out, 3u); return 0u;
            case 4u: set_arity1(out, 4u); return 0u;
            case 5u: set_arity1(out, 5u); return 0u;
            default: return kErrUnknownChannel;
            }
        }
        if (original_layout == 1983u) {
            switch (carrier_channel) {
            case 0u: set_arity2(out, 0x900000000ull); return 0u;
            case 1u: set_arity2(out, 0xA00000001ull); return 0u;
            case 2u: set_arity1(out, 2u); return 0u;
            case 3u: set_arity1(out, 3u); return 0u;
            case 4u: set_arity1(out, 4u); return 0u;
            case 5u: set_arity1(out, 5u); return 0u;
            case 7u: set_arity1(out, 7u); return 0u;
            case 8u: set_arity1(out, 8u); return 0u;
            default: return kErrUnknownChannel;
            }
        }
        return kErrUnknownLayout;
    }

    if (original_layout == 447u) {
        switch (carrier_channel) {
        case 0u: set_arity1(out, 0u); return 0u;
        case 1u: set_arity1(out, 1u); return 0u;
        case 2u: set_arity1(out, 2u); return 0u;
        case 3u: set_arity1(out, 3u); return 0u;
        case 4u: set_arity2(out, 0x700000004ull); return 0u;
        case 5u: set_arity2(out, 0x800000005ull); return 0u;
        default: return kErrUnknownChannel;
        }
    }
    if (original_layout == 1587u) {
        if (carrier_channel == 1u) {
            set_arity3(out, 0x500000001ull, 10u);
            return 0u;
        }
        if (carrier_channel == 0u) {
            set_arity3(out, 0x400000000ull, 9u);
            return 0u;
        }
        return kErrUnknownChannel;
    }
    return kErrUnknownLayout;
}

} // namespace auro3d::encode
