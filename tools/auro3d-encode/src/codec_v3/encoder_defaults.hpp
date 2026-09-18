#pragma once

#include <cstdint>
#include <array>

namespace auro3d::encode {

struct DefaultBitLine {
    std::uint32_t low = 0;
    std::uint32_t high = 0;
};

/// Direct port of encoder:default_bit_line. The returned pair is
/// the native 64-bit packed value split into low/high words.
bool codec_v3_default_bit_line(
    std::uint32_t profile,
    std::uint8_t parameter,
    DefaultBitLine& out);

/// [bit_line - 3] used by Encoder:add_ when
/// 3 <= bit_line <= 14; otherwise native falls back to 80.
bool codec_v3_bit_line_quality(
    std::uint32_t bit_line,
    std::uint32_t& quality);

/// Raw 18-dword clustering defaults returned by default_clustering.
/// Keeping the native offsets explicit avoids assigning guessed semantics to
/// fields which are consumed later by the VQ/GVM setup.
struct DefaultClustering {
    std::array<std::uint32_t, 18> words{};
};

enum class NativeGvmLearnerImplementation : std::uint8_t {
    modern,
    unavailable_mode1,
    old_fast,
};

enum class NativeClusterDeltasBackend : std::uint8_t {
    gvm,
    quantization,
};

bool codec_v3_default_clustering(
    std::uint32_t profile,
    DefaultClustering& out);

/// Field-level port of cluster_deltas:GVM:configure. The
/// learner execution remains separate, but its native configuration is kept
/// explicit so unsupported modes cannot be silently treated as defaults.
struct NativeGvmConfiguration {
    std::uint32_t mode = 0;
    NativeGvmLearnerImplementation learner =
        NativeGvmLearnerImplementation::modern;
    std::uint64_t qword_48 = 0;
    bool has_qword_48 = false;
    bool flag_40 = false;
    bool flag_41 = false;
    bool flag_64 = false;
    bool flag_65 = false;
    bool flag_66 = false;
    bool flag_67 = false;
};

bool configure_native_gvm(
    const DefaultClustering& defaults,
    NativeGvmConfiguration& out);

/// Ports backend selection in cluster_deltas:create_and_configure
/// A present mode value 3 selects Quantization; every other
/// accepted configuration selects GVM.
bool configure_native_cluster_deltas(
    const DefaultClustering& defaults,
    NativeClusterDeltasBackend& backend,
    NativeGvmConfiguration& gvm);

} // namespace auro3d:encode
