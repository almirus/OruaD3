#pragma once

#include <array>
#include <cstddef>

namespace auro3d {

class Am4hpEqualizer {
public:
    using Block = std::array<float, 32>;
    using Coefficients = std::array<float, 5>;

    struct Band {
        Coefficients coefficients{};
        float gain_db = 0.0f;
    };

    // Coefficients use native order b0,b1,b2,a1,a2. Resource extraction is
    // deliberately outside this block; no coefficients are guessed here.
    bool construct(const std::array<Band, 3>& bands,
                   std::size_t channel_count = 2u) noexcept;
    // Native Equalizer update paths replace band parameters in place and do
    // not reset the per-band/per-channel audio states.
    bool update(const std::array<Band, 3>& bands) noexcept;
    bool set_band0_gain_db(float gain_db) noexcept;
    void reset_audio_state() noexcept;
    bool process(const std::array<Block*, 2>& channels) noexcept;

private:
    struct State { float z1 = 0.0f; float z2 = 0.0f; };
    std::array<Band, 3> bands_{};
    std::array<std::array<State, 2>, 3> states_{};
    std::size_t band_count_ = 0u;
    std::size_t channel_count_ = 0u;
    bool configured_ = false;
};

}
