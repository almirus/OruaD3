#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace auro3d {

/// percent < 0 → indeterminate spinner, 0..100 → percentage bar.
using ProgressFn = std::function<void(const char* stage, int percent)>;

inline int progress_percent(std::uint64_t done, std::uint64_t total) {
    if (total == 0)
        return 100;
    if (done >= total)
        return 100;
    return static_cast<int>((done * 100ull) / total);
}

namespace console_style {
inline constexpr const char* kReset = "\033[0m";
inline constexpr const char* bold = "\033[1m";
inline constexpr const char* dim = "\033[2m";
inline constexpr const char* cyan = "\033[36m";
inline constexpr const char* bright_cyan = "\033[96m";
inline constexpr const char* green = "\033[32m";
inline constexpr const char* bright_green = "\033[92m";
inline constexpr const char* yellow = "\033[33m";
inline constexpr const char* red = "\033[31m";
inline constexpr const char* white = "\033[97m";
inline constexpr const char* bright_magenta = "\033[95m";
inline constexpr const char* hide_cursor = "\033[?25l";
inline constexpr const char* show_cursor = "\033[?25h";

inline bool stderr_is_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stderr)) != 0;
#else
    return isatty(STDERR_FILENO) != 0;
#endif
}

inline bool stdout_is_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

inline bool color_enabled_for(bool tty) {
    if (std::getenv("NO_COLOR") != nullptr)
        return false;
    if (const char* force = std::getenv("FORCE_COLOR")) {
        if (force[0] != '\0' && std::strcmp(force, "0") != 0)
            return true;
    }
    return tty;
}

inline bool color_enabled_for_stderr() {
    return color_enabled_for(stderr_is_tty());
}

inline bool color_enabled_for_stdout() {
    return color_enabled_for(stdout_is_tty());
}

inline void enable_virtual_terminal() {
#ifdef _WIN32
    for (const DWORD id : {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
        HANDLE handle = GetStdHandle(id);
        if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
            continue;
        DWORD mode = 0;
        if (!GetConsoleMode(handle, &mode))
            continue;
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(handle, mode);
    }
#endif
}

inline std::ostream& paint(std::ostream& out, bool enabled, const char* color) {
    if (enabled && color && *color)
        out << color;
    return out;
}

inline std::ostream& paint_reset(std::ostream& out, bool enabled) {
    if (enabled)
        out << kReset;
    return out;
}
} // namespace console_style

class ProgressReporter {
public:
    ProgressReporter()
        : color_(console_style::color_enabled_for_stderr())
        , tty_(console_style::stderr_is_tty()) {}

    ProgressReporter(const ProgressReporter&) = delete;
    ProgressReporter& operator=(const ProgressReporter&) = delete;

    void update(const char* stage, int percent = -1) {
        if (!stage)
            stage = "";
        std::unique_lock<std::mutex> lock(mutex_);
        const bool stage_changed = current_ != stage;
        if (stage_changed) {
            stop_spinner(lock);
            if (line_open_) {
                // Keep a completed stage line; clear only an unfinished one.
                finish_line(lock, last_percent_ >= 100);
            }
            current_ = stage;
            last_percent_ = -2;
            spinner_index_ = 0;
            last_render_ = Clock::time_point{};
        }

        if (percent < 0) {
            last_percent_ = -1;
            line_open_ = true;
            render_indeterminate();
            start_spinner(lock);
            return;
        }

        if (percent > 100)
            percent = 100;
        if (percent >= 100 && completed_stage_ == current_ && !line_open_)
            return;
        if (!stage_changed && percent == last_percent_)
            return;
        // Cap redraw rate so dense decode callbacks don't flicker the console.
        if (!stage_changed && percent < 100 && !render_due())
            return;
        last_percent_ = percent;
        line_open_ = true;
        if (percent >= 100) {
            stop_spinner(lock);
            render_done();
            finish_line(lock, true);
            completed_stage_ = current_;
        } else {
            render_percent(percent);
            start_spinner(lock);
        }
    }

    void done(const char* stage) {
        update(stage, 100);
    }

    void finish() {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_spinner(lock);
        finish_line(lock, true);
        show_cursor();
    }

    ProgressFn callback() {
        return [this](const char* stage, int percent) { update(stage, percent); };
    }

    bool color() const { return color_; }

    ~ProgressReporter() { finish(); }

private:
    using Clock = std::chrono::steady_clock;

    static constexpr const char* kSpinnerFrames[] = {
        "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    static constexpr int kSpinnerFrameCount =
        static_cast<int>(sizeof(kSpinnerFrames) / sizeof(kSpinnerFrames[0]));
    static constexpr int kBarWidth = 24;
    static constexpr int kStageWidth = 16; // longest stage label width
    static constexpr auto kMinRedrawInterval = std::chrono::milliseconds(50);

    void start_spinner(std::unique_lock<std::mutex>&) {
        if (spinner_running_)
            return;
        spinner_stop_.store(false, std::memory_order_release);
        spinner_running_ = true;
        spinner_thread_ = std::thread([this] { spinner_loop(); });
    }

    void stop_spinner(std::unique_lock<std::mutex>& lock) {
        if (!spinner_running_)
            return;
        spinner_stop_.store(true, std::memory_order_release);
        lock.unlock();
        if (spinner_thread_.joinable())
            spinner_thread_.join();
        lock.lock();
        spinner_running_ = false;
    }

    void spinner_loop() {
        using namespace std::chrono_literals;
        while (!spinner_stop_.load(std::memory_order_acquire)) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (line_open_ && last_percent_ >= -1 && last_percent_ < 100) {
                    ++spinner_index_;
                    if (last_percent_ < 0)
                        render_indeterminate();
                    else
                        render_percent(last_percent_);
                }
            }
            std::this_thread::sleep_for(100ms);
        }
    }

    bool render_due() const {
        if (last_render_ == Clock::time_point{})
            return true;
        return Clock::now() - last_render_ >= kMinRedrawInterval;
    }

    void hide_cursor() {
        if (!tty_ || cursor_hidden_)
            return;
        std::cerr << console_style::hide_cursor;
        cursor_hidden_ = true;
    }

    void show_cursor() {
        if (!cursor_hidden_)
            return;
        std::cerr << console_style::show_cursor << std::flush;
        cursor_hidden_ = false;
    }

    void emit_line(const std::string& line) {
        hide_cursor();
        // Overwrite in place: never clear-before-write (that blanks a frame and flickers).
        std::cerr << '\r' << line << "\033[K" << std::flush;
        last_render_ = Clock::now();
        line_open_ = true;
    }

    void append_paint(std::ostream& out, const char* color) const {
        console_style::paint(out, color_, color);
    }

    void append_reset(std::ostream& out) const {
        console_style::paint_reset(out, color_);
    }

    void append_stage(std::ostream& out) const {
        append_paint(out, console_style::bold);
        append_paint(out, console_style::white);
        out << current_;
        append_reset(out);
        if (static_cast<int>(current_.size()) < kStageWidth)
            out << std::string(static_cast<std::size_t>(kStageWidth) - current_.size(), ' ');
    }

    void render_indeterminate() {
        std::ostringstream oss;
        const char* frame = kSpinnerFrames[spinner_index_ % kSpinnerFrameCount];
        append_paint(oss, console_style::bright_cyan);
        oss << frame;
        append_reset(oss);
        oss << ' ';
        append_stage(oss);
        oss << ' ';
        append_paint(oss, console_style::dim);
        oss << "working";
        append_reset(oss);
        emit_line(oss.str());
    }

    void render_percent(int percent) {
        std::ostringstream oss;
        const char* frame = kSpinnerFrames[spinner_index_++ % kSpinnerFrameCount];
        const std::string bar = make_bar(percent);
        append_paint(oss, console_style::bright_cyan);
        oss << frame;
        append_reset(oss);
        oss << ' ';
        append_stage(oss);
        oss << ' ';
        append_paint(oss, console_style::cyan);
        oss << bar;
        append_reset(oss);
        oss << ' ';
        append_paint(oss, console_style::bold);
        append_paint(oss, console_style::bright_cyan);
        oss << percent << '%';
        append_reset(oss);
        emit_line(oss.str());
    }

    void render_done() {
        std::ostringstream oss;
        append_paint(oss, console_style::bright_green);
        oss << "✔";
        append_reset(oss);
        oss << ' ';
        append_stage(oss);
        oss << ' ';
        append_paint(oss, console_style::green);
        oss << make_bar(100);
        append_reset(oss);
        oss << ' ';
        append_paint(oss, console_style::bright_green);
        oss << "100%";
        append_reset(oss);
        emit_line(oss.str());
    }

    static std::string make_bar(int percent) {
        percent = std::max(0, std::min(100, percent));
        const int filled = (percent * kBarWidth + 50) / 100;
        std::string bar;
        bar.reserve(static_cast<std::size_t>(kBarWidth) + 2);
        bar.push_back('[');
        for (int i = 0; i < kBarWidth; ++i)
            bar += (i < filled) ? "█" : "░";
        bar.push_back(']');
        return bar;
    }

    void finish_line(std::unique_lock<std::mutex>&, bool keep_content) {
        if (!line_open_)
            return;
        if (!keep_content)
            std::cerr << '\r' << "\033[K";
        std::cerr << '\n' << std::flush;
        line_open_ = false;
        last_percent_ = -2;
    }

    bool color_ = false;
    bool tty_ = false;
    bool cursor_hidden_ = false;
    std::string current_;
    std::string completed_stage_;
    int last_percent_ = -2;
    int spinner_index_ = 0;
    bool line_open_ = false;
    bool spinner_running_ = false;
    std::atomic<bool> spinner_stop_{false};
    std::thread spinner_thread_;
    std::mutex mutex_;
    Clock::time_point last_render_{};
};

} // namespace auro3d
