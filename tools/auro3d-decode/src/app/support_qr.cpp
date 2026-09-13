#include "support_qr.hpp"

#include "progress.hpp"

#include "../../third_party/qrcodegen/qrcodegen.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace auro3d {
namespace {

constexpr const char* kSupportCard = "2200 7009 5155 4582";
constexpr const char* kSupportBitcoin =
    "bc1qu5e8gj2z0ezge2y6k88n0mlevdgum72mqmdd9j";
constexpr const char* kSupportEthereum =
    "0x5AA8B619B4a1C598b48F370F48602E8E327c9E3D";

constexpr const char kFullBlock[] = u8"\u2588";
constexpr const char kUpperHalf[] = u8"\u2580";
constexpr const char kLowerHalf[] = u8"\u2584";
constexpr const char* kAnsiQrPalette = "\033[30;47m";
constexpr const char* kAnsiReset = "\033[0m";

std::vector<std::string> render_terminal_qr_lines(
    const qrcodegen::QrCode& code,
    bool color_enabled,
    int quiet_zone = 4) {
    const int size = code.getSize();
    const int total = size + 2 * quiet_zone;
    std::vector<std::string> lines;
    lines.reserve(static_cast<std::size_t>((total + 1) / 2));

    const auto is_dark = [&](int x, int y) -> bool {
        const int mx = x - quiet_zone;
        const int my = y - quiet_zone;
        if (mx < 0 || my < 0 || mx >= size || my >= size) {
            return false;
        }
        return code.getModule(mx, my);
    };

    for (int y = 0; y < total; y += 2) {
        std::string line;
        line.reserve(static_cast<std::size_t>(total) * 4U + 16U);
        if (color_enabled) {
            line += kAnsiQrPalette;
        }
        for (int x = 0; x < total; ++x) {
            const bool top = is_dark(x, y);
            const bool bottom = (y + 1 < total) && is_dark(x, y + 1);
            if (top && bottom) {
                line += kFullBlock;
            } else if (top) {
                line += kUpperHalf;
            } else if (bottom) {
                line += kLowerHalf;
            } else {
                line += ' ';
            }
        }
        if (color_enabled) {
            line += kAnsiReset;
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

} // namespace

void print_support_author(std::ostream& out, bool color) {
    out << "Поддержать автора:  ";
    console_style::paint(out, color, console_style::bold);
    console_style::paint(out, color, console_style::yellow);
    out << kSupportCard;
    console_style::paint_reset(out, color);
    out << "\nBTC QR: " << kSupportBitcoin
        << "\nETH QR: " << kSupportEthereum << '\n';

    try {
        const qrcodegen::QrCode btc = qrcodegen::QrCode::encodeText(
            kSupportBitcoin, qrcodegen::QrCode::Ecc::MEDIUM);
        const qrcodegen::QrCode eth = qrcodegen::QrCode::encodeText(
            kSupportEthereum, qrcodegen::QrCode::Ecc::MEDIUM);
        const auto btc_lines = render_terminal_qr_lines(btc, color);
        const auto eth_lines = render_terminal_qr_lines(eth, color);
        const std::size_t line_count =
            btc_lines.size() > eth_lines.size()
                ? btc_lines.size()
                : eth_lines.size();
        for (std::size_t i = 0; i < line_count; ++i) {
            if (i < btc_lines.size()) {
                out << btc_lines[i];
            }
            out << "    ";
            if (i < eth_lines.size()) {
                out << eth_lines[i];
            }
            out << '\n';
        }
        console_style::paint_reset(out, color);
    } catch (const std::exception&) {
        // Keep the donation address visible even if QR generation fails.
    }

    out << '\n';
}

} // namespace auro3d
