#pragma once

#include "am4hp_equalizer.hpp"

#include <array>

namespace auro3d {

// Exact float32 coefficient profiles captured from the matching native
// Artist Connection libauro.so. They are intentionally exposed as
// data only; selecting a profile still belongs to the AM4HP layout/mode
// constructor and is not inferred from an arbitrary channel count.
std::array<Am4hpEqualizer::Band, 3> am4hp_native_eq_hl_hr_48000() noexcept;
std::array<Am4hpEqualizer::Band, 3> am4hp_native_eq_hls_hrs_48000() noexcept;
std::array<Am4hpEqualizer::Band, 3>
am4hp_native_surround_eq_hl_hr_48000() noexcept;
std::array<Am4hpEqualizer::Band, 3>
am4hp_native_surround_eq_hls_hrs_48000() noexcept;

}
