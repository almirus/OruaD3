namespace {

constexpr std::uintptr_t kV3DecoderWrapperOffAllowDecoding = 2508u;
constexpr std::uintptr_t kEb5a0BlobOffDecideDecodeGate = 252124u;

std::uint64_t codec_v3_codec_decoder_from_ctx(std::uint64_t ctx) {
    if (ctx == 0u)
        return 0u;
    const auto* blob = reinterpret_cast<const std::uint8_t*>(ctx);
    const std::uint64_t codec = *reinterpret_cast<const std::uint64_t*>(blob);
    if (codec == 0u)
        return 0u;
    return codec;
}

std::uint32_t codec_v3_block_info_segment_count_from_ctx(std::uint64_t ctx) {
    const std::uint64_t codec = codec_v3_codec_decoder_from_ctx(ctx);
    if (codec == 0u)
        return 0u;
    const auto* og = reinterpret_cast<const std::uint8_t*>(
        static_cast<std::uintptr_t>(codec + auro_codec_v3::kDecoder_off_OutputGenerator));
    const std::uint64_t block_info = *reinterpret_cast<const std::uint64_t*>(og + kOgOff_segment_ctx_ptr);
    if (block_info == 0u)
        return 0u;
    return *reinterpret_cast<const std::uint32_t*>(block_info + 24u);
}

std::uint32_t codec_v3_allow_decoding_from_decoder(const std::uint8_t* decoder_base) {
    (void)decoder_base;
    return 1u;
}

std::uint32_t codec_v3_allow_decoding_from_ctx(std::uint64_t ctx) {
    if (ctx == 0u)
        return 1u;
    const auto* blob = reinterpret_cast<const std::uint8_t*>(ctx);
    const std::uint32_t wrapper_gate =
        *reinterpret_cast<const std::uint32_t*>(blob + kV3DecoderWrapperOffAllowDecoding);
    if (wrapper_gate <= 1u)
        return wrapper_gate;
    return *reinterpret_cast<const std::uint32_t*>(blob + kEb5a0BlobOffDecideDecodeGate);
}

struct DownmixToPartial {
    std::uint32_t mask = 0;
    bool ok = false;
};

constexpr std::size_t kDownmixPlanIntCount = 0x280u / sizeof(std::int32_t);

namespace {

struct DownmixPlanMut {
    std::int32_t* plan = nullptr;
    std::uint64_t* row_count = nullptr;
    std::uint32_t* out_mask = nullptr;

    explicit DownmixPlanMut(std::int32_t* p)
        : plan(p)
        , row_count(reinterpret_cast<std::uint64_t*>(p))
        , out_mask(reinterpret_cast<std::uint32_t*>(p + 159)) {}

    bool push_qd(std::uint64_t qword, std::int32_t dword) {
        const std::uint64_t idx = *row_count;
        if (idx > 0x33u)
            return false;
        *row_count = idx + 1u;
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(idx);
        *reinterpret_cast<std::uint64_t*>(plan + off) = qword;
        plan[off + 2] = dword;
        return true;
    }

    bool push_dq(std::int32_t dword0, std::uint64_t qword1) {
        const std::uint64_t idx = *row_count;
        if (idx > 0x33u)
            return false;
        *row_count = idx + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 8u + 12u * idx;
        *reinterpret_cast<std::int32_t*>(row_base) = dword0;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = qword1;
        return true;
    }

    void patch_mask(std::uint32_t or_bits, std::uint32_t and_mask) {
        *out_mask = or_bits | (*out_mask & and_mask);
    }

    bool push_dq_after_two(
        std::uint64_t first_row_idx,
        std::int32_t dword0,
        std::uint64_t qword1,
        std::uint32_t& src_mask) {
        *row_count = first_row_idx + 2u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 4u * (3u * first_row_idx + 5u);
        *reinterpret_cast<std::int32_t*>(row_base) = dword0;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = qword1;
        (void)src_mask;
        return true;
    }
};

constexpr std::uint64_t kDmPair15873 = 0x0000003E00000001uLL;
constexpr std::uint64_t kDmPair14614 = 0x0000003900000016uLL;
constexpr std::uint64_t kDmPair14085 = 0x0000003700000005uLL;
constexpr std::uint64_t kDmPair15382 = 0x0000003C00000016uLL;
constexpr std::uint64_t kDmPair3329 = 0x0000000D00000001uLL;
constexpr std::uint64_t kDmPair2821 = 0x0000000B00000005uLL;
constexpr std::uint64_t kDmPair268 = 0x000000010000000CuLL;
constexpr std::uint64_t kDmPair1802 = 0x000000070000000AuLL;
constexpr std::uint64_t kDmPair1294 = 0x000000050000000EuLL;
constexpr std::uint64_t kDmPair11784 = 0x0000002E00000008uLL;
constexpr std::uint64_t kDmPair11269 = 0x0000002C00000005uLL;
constexpr std::uint64_t kDmPair13569 = 0x0000003500000001uLL;
constexpr std::uint64_t kDmPair18950 = 0x0000004A00000056uLL;
constexpr std::uint64_t kDmPair22790 = 0x0000005900000056uLL;
constexpr std::uint64_t kDmPair17925 = 0x0000004600000005uLL;
constexpr std::uint64_t kDmPair6146 = 0x0000001800000002uLL;
constexpr std::uint64_t kDmPair6917 = 0x0000001B00000005uLL;
constexpr std::uint64_t kDmPair7941 = 0x0000001F00000005uLL;

bool downmix_label101_tgt_layout_rejected(std::uint32_t tgt_layout) {
    // masked tgt equals in active lanes.
    if ((tgt_layout & 0x8000u) != 0u)
        return false;
    if ((tgt_layout & 0x300000u) != 0x3000000u)
        return false;
    if ((tgt_layout & 0x600000u) != 0x600000u)
        return false;
    return (tgt_layout & 0x6u) == 0u;
}

bool auro_downmix_v1_plan_apply_label274(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * anchor_row + 5u);
    *reinterpret_cast<std::int32_t*>(row_base) = dword0;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = pair_qword;
    // Native applies the second route to a2[159], which already
    // contains the mask update made by the first route in the pair.
    src_mask = or_bits | (*dp.out_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool auro_downmix_v1_plan_apply_label100_write(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * anchor_row + 2u);
    *reinterpret_cast<std::int32_t*>(row_base) = dword0;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = pair_qword;
    src_mask = or_bits | (src_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool auro_downmix_v1_plan_apply_label100(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    *dp.row_count = anchor_row + 1u;
    return auro_downmix_v1_plan_apply_label100_write(
        dp, anchor_row, dword0, pair_qword, or_bits, and_mask, src_mask);
}

constexpr std::uint64_t kDmPair8975 = 0x000000230000000FuLL;
constexpr std::uint64_t kDmPair9486 = 0x000000250000000EuLL;
constexpr std::uint64_t kDmPair12298 = 0x000000300000000AuLL;
constexpr std::uint64_t kDmPair12810 = 0x000000320000000AuLL;
constexpr std::uint64_t kDmPair3869 = 0x0000000F0000001DuLL;
constexpr std::uint64_t kDmPair4107 = 0x000000100000000BuLL;
constexpr std::uint64_t kDmPair4878 = 0x000000130000000EuLL;
constexpr std::uint64_t kDmPair5902 = 0x0000001700000006uLL;
constexpr std::uint64_t kDmPair21249 = 0x0000005300000001uLL;
constexpr std::uint64_t kDmPair22282 = 0x000000570000000AuLL;
constexpr std::uint64_t kDmPair20481 = 0x0000005000000001uLL;
constexpr std::uint64_t kDmPair20738 = 0x0000005100000002uLL;
constexpr std::uint64_t kDmPair20766 = 0x000000510000001EuLL;

bool
auro_downmix_v1_plan_finalize_label275(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint);

bool
auro_downmix_v1_plan_finish_label275(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
    if (!
auro_downmix_v1_plan_finalize_label275(plan, tgt_layout, tgt_class_hint))
        return false;
    if (engine_complete)
        *engine_complete = true;
    return true;
}

bool
auro_downmix_v1_plan_label257(
    DownmixPlanMut& dp,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
  // shared tail before.
  if ((~src_mask & 0x30u) != 0u)
    return
auro_downmix_v1_plan_finish_label275(
        dp.plan, tgt_layout, tgt_class_hint, engine_complete);

  const std::uint64_t r0 = *dp.row_count;
  if (r0 > 0x33u)
    return false;
  if (!dp.push_qd(4u, 84))
    return false;
  dp.patch_mask(1u, 0xFFFFFFEEu);
  if (r0 == 51u)
    return false;
  if (!dp.push_qd(0x100000005uLL, 85))
    return false;
  dp.patch_mask(2u, 0xFFFFFFDDu);
  if (r0 > 0x31u)
    return false;
  if (!dp.push_qd(0x900000004uLL, 86))
    return false;
  dp.patch_mask(0x200u, 0xFFFFFDEFu);
  if (r0 == 49u)
    return false;
  *dp.row_count = r0 + 4u;
  if (!auro_downmix_v1_plan_apply_label274(
          dp, r0 + 3u, 5, kDmPair22282, 1024u, 0xFFFFFBFFu, src_mask))
    return false;
  return
auro_downmix_v1_plan_finish_label275(
      dp.plan, tgt_layout, tgt_class_hint, engine_complete);
}

bool
auro_downmix_v1_plan_label209(
    DownmixPlanMut& dp,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
    if (engine_complete)
        *engine_complete = false;
    const auto finish = [&]() {
        return
auro_downmix_v1_plan_finish_label275(
            dp.plan, tgt_layout, tgt_class_hint, engine_complete);
    };
    const std::uint32_t v11 = inv_tgt;
    const std::uint32_t tgt_lo = tgt_layout & 3u;
    const std::uint32_t v147 = tgt_layout & 0x30u;
    const std::uint32_t v73 = tgt_layout & 0x600u;

    if ((tgt_layout & 0x40u) != 0u) {
        const bool v154 = ((~src_mask & 0x180u) != 0u) || ((v11 & 0x180u) == 0u);
        if (v147 == 48u) {
            if (v154)
                return finish();
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x400000007uLL, 71))
                return false;
            dp.patch_mask(0x10u, 0xFFFFFF6Fu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x500000008uLL, 72))
                return false;
            dp.patch_mask(0x20u, 0xFFFFFEDFu);
            if (r0 > 0x31u)
                return false;
            if (!dp.push_qd(0x600000007uLL, 73))
                return false;
            dp.patch_mask(0x40u, 0xFFFFFF3Fu);
            if (r0 == 49u)
                return false;
            *dp.row_count = r0 + 4u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0 + 3u, 8, kDmPair18950, 64u, 0xFFFFFEBFu, src_mask))
                return false;
        } else {
            if (v154) {
                if ((~src_mask & 0x30u) != 0u)
                    return finish();
            } else {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x400000007uLL, 69))
                    return false;
                dp.patch_mask(0x10u, 0xFFFFFF6Fu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x500000008uLL, 70))
                    return false;
                src_mask = src_mask & 0xFFFFFEDFu | 0x20u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
                if ((~src_mask & 0x30u) != 0u)
                    return finish();
            }
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x600000004uLL, 88))
                return false;
            dp.patch_mask(0x40u, 0xFFFFFFAFu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0, 5, kDmPair22790, 64u, 0xFFFFFF9Fu, src_mask))
                return false;
        }
        return finish();
    }

    if (v147 == 48u) {
        if ((src_mask & 0x40u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x700000006uLL, 77))
                return false;
            dp.patch_mask(0x80u, 0xFFFFFF3Fu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x800000006uLL, 78))
                return false;
            src_mask = src_mask & 0xFFFFFEBFu | 0x100u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
            if ((v11 & 0x180u) == 0u)
                return finish();
        } else if ((v11 & 0x180u) == 0u) {
            return finish();
        }
        if ((src_mask & 0x180u) != 0x180u)
            return finish();
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x400000007uLL, 69))
            return false;
        dp.patch_mask(0x10u, 0xFFFFFF6Fu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label274(
                dp, r0, 8, kDmPair17925, 32u, 0xFFFFFEDFu, src_mask))
            return false;
        return finish();
    }

    if (v73 != 1536u) {
        if ((v11 & 0x180u) != 0u && (src_mask & 0x180u) == 0x180u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(7u, 75))
                return false;
            dp.patch_mask(1u, 0xFFFFFF7Eu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x100000008uLL, 76))
                return false;
            src_mask = src_mask & 0xFFFFFEFDu | 2u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((src_mask & 0x40u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (tgt_lo == 3u) {
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(6u, 79))
                    return false;
                dp.patch_mask(1u, 0xFFFFFFBEu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 6, kDmPair20481, 2u, 0xFFFFFFBDu, src_mask))
                    return false;
            } else if (tgt_class_hint != 0u) {
                if (r0 > 0x33u)
                    return false;
                *dp.row_count = r0 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * r0 + 2u);
                *reinterpret_cast<std::int32_t*>(row_base) = 6;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = kDmPair20738;
                src_mask = 4u | (src_mask & 0xFFFFFFBBu);
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            } else {
                if (r0 > 0x33u)
                    return false;
                *dp.row_count = r0 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * r0 + 2u);
                *reinterpret_cast<std::int32_t*>(row_base) = 6;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = kDmPair20766;
                src_mask = 0x40000000u | (src_mask & 0xBFFFFFBFu);
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
        }

        if ((~src_mask & 0x30u) != 0u)
            return finish();

        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(4u, 82))
            return false;
        dp.patch_mask(1u, 0xFFFFFFEEu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label274(
                dp, r0, 5, kDmPair21249, 2u, 0xFFFFFFDDu, src_mask))
            return false;
        return finish();
    }

    if ((src_mask & 0x40u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x700000006uLL, 77))
            return false;
        dp.patch_mask(0x80u, 0xFFFFFF3Fu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0x800000006uLL, 78))
            return false;
        src_mask = src_mask & 0xFFFFFEBFu | 0x100u;
        dp.patch_mask(src_mask, 0xFFFFFFFFu);
        if ((v11 & 0x180u) == 0u)
            return
auro_downmix_v1_plan_label257(
                dp, src_mask, tgt_layout, tgt_class_hint, engine_complete);
    } else if ((v11 & 0x180u) == 0u) {
        return
auro_downmix_v1_plan_label257(
            dp, src_mask, tgt_layout, tgt_class_hint, engine_complete);
    }

    if ((src_mask & 0x180u) == 0x180u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x400000007uLL, 69))
            return false;
        dp.patch_mask(0x10u, 0xFFFFFF6Fu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0x500000008uLL, 70))
            return false;
        src_mask = src_mask & 0xFFFFFEDFu | 0x20u;
        dp.patch_mask(src_mask, 0xFFFFFFFFu);
    }

    return
auro_downmix_v1_plan_label257(
        dp, src_mask, tgt_layout, tgt_class_hint, engine_complete);
}

bool
auro_downmix_v1_plan_label101_v73_1536(
    DownmixPlanMut& dp,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt) {
    const std::int32_t eng4 = engine ? engine[4] : 0;
    const std::int32_t eng5 = engine ? engine[5] : 0;
    const std::uint32_t v11 = inv_tgt;

    if ((v11 & 0x6000u) != 0u) {
        // v93 = 1; if ((v11 & 0x30) == 0) v93 = eng4;
        std::uint32_t v93 = 1u;
        if ((v11 & 0x30u) == 0u)
            v93 = static_cast<std::uint32_t>(eng4);
        if (v93 == 0u) {
            if ((tgt_layout & 0x8000u) == 0u && (src_mask & 0x8000u) != 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x60000000FuLL, 40))
                    return false;
                src_mask = src_mask & 0xFFFF7FBFu | 0x40u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((v11 & 0x30000u) != 0u && (src_mask & 0x30000u) == 0x30000u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x700000010uLL, 38))
                    return false;
                dp.patch_mask(0x80u, 0xFFFEFF7Fu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x800000011uLL, 39))
                    return false;
                src_mask = src_mask & 0xFFFDFEFFu | 0x100u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((~src_mask & 0x6000u) == 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (eng5 == 1) {
                    if (r0 > 0x33u)
                        return false;
                    if (!dp.push_qd(0x70000000DuLL, 45))
                        return false;
                    dp.patch_mask(0x80u, 0xFFFFDF7Fu);
                    if (r0 == 51u)
                        return false;
                    *dp.row_count = r0 + 2u;
                    if (!auro_downmix_v1_plan_apply_label274(
                            dp, r0, 14, kDmPair11784, 256u, 0xFFFFBEFFu, src_mask))
                        return false;
                } else {
                    if (r0 > 0x33u)
                        return false;
                    if (!dp.push_qd(0x40000000DuLL, 43))
                        return false;
                    dp.patch_mask(0x10u, 0xFFFFDFEFu);
                    if (r0 == 51u)
                        return false;
                    *dp.row_count = r0 + 2u;
                    if (!auro_downmix_v1_plan_apply_label274(
                            dp, r0, 14, kDmPair11269, 32u, 0xFFFFBFDFu, src_mask))
                        return false;
                }
            }
        } else if (v93 != 1u) {
            return false;
        } else {
            if ((tgt_layout & 0x8000u) == 0u && (src_mask & 0x8000u) != 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x100000000FuLL, 41))
                    return false;
                dp.patch_mask(0x10000u, 0xFFFE7FFFu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x110000000FuLL, 42))
                    return false;
                src_mask = src_mask & 0xFFFD7FFFu | 0x20000u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((v11 & 0x30000u) != 0u && (src_mask & 0x30000u) == 0x30000u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0xD00000010uLL, 36))
                    return false;
                dp.patch_mask(0x2000u, 0xFFFEDFFFu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0xE00000011uLL, 37))
                    return false;
                src_mask = src_mask & 0xFFFDBFFFu | 0x4000u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((~src_mask & 0x6000u) == 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x90000000DuLL, 47))
                    return false;
                dp.patch_mask(0x200u, 0xFFFFDDFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 14, kDmPair12298, 1024u, 0xFFFFBBFFu, src_mask))
                    return false;
            }
        }
    } else if ((tgt_layout & 0x8000u) != 0u) {
        if ((v11 & 0x30000u) != 0u && (src_mask & 0x30000u) == 0x30000u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0xD00000010uLL, 32))
                return false;
            dp.patch_mask(0x2000u, 0xFFFEDFFFu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0xE00000011uLL, 33))
                return false;
            dp.patch_mask(0x4000u, 0xFFFDBFFFu);
            if (r0 > 0x31u)
                return false;
            if (!dp.push_qd(0xF00000010uLL, 34))
                return false;
            dp.patch_mask(0x8000u, 0xFFFE7FFFu);
            if (r0 == 49u)
                return false;
            *dp.row_count = r0 + 4u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0 + 3u, 17, kDmPair8975, 0x8000u, 0xFFFD7FFFu, src_mask))
                return false;
        }
    } else {
        if ((src_mask & 0x8000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x100000000FuLL, 41))
                return false;
            dp.patch_mask(0x10000u, 0xFFFE7FFFu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x110000000FuLL, 42))
                return false;
            src_mask = src_mask & 0xFFFD7FFFu | 0x20000u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
            if ((v11 & 0x30000u) == 0u)
                goto label_203;
        } else if ((v11 & 0x30000u) == 0u) {
            goto label_203;
        }
        if ((src_mask & 0x30000u) == 0x30000u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0xD00000010uLL, 36))
                return false;
            dp.patch_mask(0x2000u, 0xFFFEDFFFu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0, 17, kDmPair9486, 0x4000u, 0xFFFD9FFFu, src_mask))
                return false;
        }
    }

label_203:
    if ((tgt_layout & 0x800u) == 0u && (src_mask & 0x800u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x90000000BuLL, 49))
            return false;
        dp.patch_mask(0x200u, 0xFFFFF5FFu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label274(
                dp, r0, 11, kDmPair12810, 1024u, 0xFFFFF3FFu, src_mask))
            return false;
    }
    return true;
}

bool
auro_downmix_v1_plan_height_post_label101(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt,
    bool* goto_label101) {
    if (goto_label101)
        *goto_label101 = false;
    DownmixPlanMut dp(plan);
    const std::int32_t eng0 = engine ? engine[0] : 0;

    // tgt 0x30000000 -> (before eng0/eng1).
    if ((tgt_layout & 0x30000000u) == 0x30000000u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x1C0000000CuLL, 14))
            return false;
        dp.patch_mask(0x10000000u, 0xEFFFEFFFu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0 + 1u, 12, kDmPair3869, 0x20000000u, 0xDFFEFBFFu, src_mask))
            return false;
        if (goto_label101)
            *goto_label101 = true;
        return true;
    }

    // eng0 only when (inv_tgt & 0x600) == 0; else fall through to eng1.
    if ((inv_tgt & 0x600u) != 0u)
        return true;

    if ((inv_tgt & 0x6000u) != 0u || eng0 == 2) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        *dp.row_count = r0 + 1u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0, 12, kDmPair4107, 2048u, 0xFFFFE7FFu, src_mask))
            return false;
        if (goto_label101)
            *goto_label101 = true;
        return true;
    }

    // eng0==1 without tgt&0x800 falls through to eng0==0 path.
    if (eng0 == 1 && (tgt_layout & 0x800u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0xB0000000CuLL, 17))
            return false;
        dp.patch_mask(0x800u, 0xFFFFE7FFu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0xD0000000CuLL, 18))
            return false;
        dp.patch_mask(0x2000u, 0xFFFFCFFFu);
        if (r0 > 0x31u)
            return false;
        *dp.row_count = r0 + 3u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0 + 2u, 12, kDmPair4878, 0x4000u, 0xFFFFAFFFu, src_mask))
            return false;
        if (goto_label101)
            *goto_label101 = true;
        return true;
    }

    if (eng0 != 0 && eng0 != 1)
        return false;

    const std::uint64_t r0 = *dp.row_count;
    if (r0 > 0x33u)
        return false;
    if (!dp.push_qd(0x90000000CuLL, 20))
        return false;
    dp.patch_mask(0x200u, 0xFFFFEDFFu);
    if (r0 == 51u)
        return false;
    if (!dp.push_qd(0xA0000000CuLL, 21))
        return false;
    dp.patch_mask(0x400u, 0xFFFFEBFFu);
    if (r0 > 0x31u)
        return false;
    if (!dp.push_qd(0xD0000000CuLL, 22))
        return false;
    dp.patch_mask(0x2000u, 0xFFFFCFFFu);
    if (r0 == 49u)
        return false;
    *dp.row_count = r0 + 4u;
    if (!auro_downmix_v1_plan_apply_label100_write(
            dp, r0 + 3u, 12, kDmPair5902, 0x4000u, 0xFFFFAFFFu, src_mask))
        return false;
    if (goto_label101)
        *goto_label101 = true;
    return true;
}

bool
auro_downmix_v1_plan_height_tail_eng1(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    bool* goto_label101_out) {
    if (goto_label101_out)
        *goto_label101_out = false;
    DownmixPlanMut dp(plan);
    const std::int32_t eng1 = engine ? engine[1] : 0;
    const std::uint32_t inv_tgt = ~tgt_layout;

    if ((inv_tgt & 0x30u) != 0u || eng1 == 2) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        // v66 = -4101 → 0xFFFFEFFB (clears Top+C, not RB).
        if (!auro_downmix_v1_plan_apply_label100(
                dp, r0, 12, kDmPair6146, 4u, 0xFFFFEFFBu, src_mask))
            return false;

        if (goto_label101_out)
            *goto_label101_out = true;
        return true;
    }

    // eng1==1 with tgt&4 -> 6917; without bit4 falls through to eng1==0.
    if (eng1 == 1 && (tgt_layout & 4u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x20000000CuLL, 25))
            return false;
        dp.patch_mask(4u, 0xFFFFEFFBu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0x40000000CuLL, 26))
            return false;
        dp.patch_mask(0x10u, 0xFFFFEFEFu);
        if (r0 > 0x31u)
            return false;
        *dp.row_count = r0 + 3u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0 + 2u, 12, kDmPair6917, 32u, 0xFFFFEFDFu, src_mask))
            return false;
        if (goto_label101_out)
            *goto_label101_out = true;
        return true;
    }

    if (eng1 != 0 && eng1 != 1)
        return false;

    const std::uint64_t r0 = *dp.row_count;
    if (r0 > 0x33u)
        return false;
    if (!dp.push_qd(12u, 28))
        return false;
    dp.patch_mask(1u, 0xFFFFEFFEu);
    if (r0 == 51u)
        return false;
    if (!dp.push_qd(0x10000000CuLL, 29))
        return false;
    dp.patch_mask(2u, 0xFFFFEFFDu);
    if (r0 > 0x31u)
        return false;
    if (!dp.push_qd(0x40000000CuLL, 30))
        return false;
    dp.patch_mask(0x10u, 0xFFFFEFEFu);
    if (r0 == 49u)
        return false;
    *dp.row_count = r0 + 4u;
    if (!auro_downmix_v1_plan_apply_label100_write(
            dp, r0 + 3u, 12, kDmPair7941, 32u, 0xFFFFEFDFu, src_mask))
        return false;
    if (goto_label101_out)
        *goto_label101_out = true;
    return true;
}

bool
auro_downmix_v1_plan_label101(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
    DownmixPlanMut dp(plan);
    const std::int32_t eng5 = engine ? engine[5] : 0;

    const std::uint32_t v73 = tgt_layout & 0x600u;
    if (v73 != 1536u) {
        if (downmix_label101_tgt_layout_rejected(tgt_layout))
            return false;

        if ((~src_mask & 0x30000u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x700000010uLL, 38))
                return false;
            dp.patch_mask(0x80u, 0xFFFEFF7Fu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x800000011uLL, 39))
                return false;
            src_mask = src_mask & 0xFFFDFEFFu | 0x100u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((src_mask & 0x8000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x60000000FuLL, 40))
                return false;
            src_mask = src_mask & 0xFFFF7FBFu | 0x40u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((~src_mask & 0x6000u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (eng5 == 1) {
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x70000000DuLL, 45))
                    return false;
                dp.patch_mask(0x80u, 0xFFFFDF7Fu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 14, kDmPair11784, 256u, 0xFFFFBEFFu, src_mask))
                    return false;
            } else {
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x40000000DuLL, 43))
                    return false;
                dp.patch_mask(0x10u, 0xFFFFDFEFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 14, kDmPair11269, 32u, 0xFFFFBFDFu, src_mask))
                    return false;
            }
        }

        if ((src_mask & 0x800u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x20000000BuLL, 51))
                return false;
            src_mask = src_mask & 0xFFFFF7FBu | 4u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((~src_mask & 0x600u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(9u, 52))
                return false;
            dp.patch_mask(1u, 0xFFFFFDFEu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0, 10, kDmPair13569, 2u, 0xFFFFFBFDu, src_mask))
                return false;
        }

        return
auro_downmix_v1_plan_label209(
            dp, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete);
    }

    if (!
auro_downmix_v1_plan_label101_v73_1536(
            dp, engine, src_mask, tgt_layout, inv_tgt))
        return false;
    return
auro_downmix_v1_plan_label209(
        dp, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete);
}

bool auro_downmix_v1_plan_apply_label35(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    if (!dp.push_dq_after_two(anchor_row, dword0, pair_qword, src_mask))
        return false;
    src_mask = or_bits | (src_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool auro_downmix_v1_plan_apply_label29(
    DownmixPlanMut& dp,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    const std::uint64_t rows = *dp.row_count;
    if (rows > 0x33u)
        return false;
    auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 8u + 12u * rows;
    *reinterpret_cast<std::int32_t*>(row_base) = dword0;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = pair_qword;
    src_mask = or_bits | (src_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool
auro_downmix_v1_plan_height_preprocess(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t src_layout,
    std::uint32_t tgt_layout,
    bool* engine_complete) {
    if (engine_complete)
        *engine_complete = false;
    DownmixPlanMut dp(plan);
    const std::uint32_t inv_tgt = ~tgt_layout;
    std::uint32_t src_mask = *dp.out_mask;
    const std::int32_t eng2 = engine ? engine[2] : 0;
    const std::int32_t eng3 = engine ? engine[3] : 0;

    const std::uint32_t v13 = src_mask & 0x3000000u;
    if ((inv_tgt & 0x600000u) != 0u) {
        if (v13 == 50331648u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(0x400000018uLL, 54))
                return false;
            dp.patch_mask(0x10u, 0xFEFFFFEFu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x500000019uLL, 55))
                return false;
            src_mask = src_mask & 0xFDFFFFDFu | 0x20u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }
        if ((src_mask & 0x800000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(0x200000017uLL, 58))
                return false;
            src_mask = src_mask & 0xFF7FFFFBu | 4u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
            if (r0 == 51u)
                return false;
        }
        if ((~src_mask & 0x600000u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(21u, 61))
                return false;
            dp.patch_mask(1u, 0xFFDFFFFEu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label35(
                    dp, r0, 22, kDmPair15873, 2u, 0xFFBFFFFDu, src_mask))
                return false;
        }
    } else {
        if (v13 == 50331648u && (inv_tgt & 0x3000000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            std::uint64_t pair_qword = 0u;
            std::uint32_t or_bits = 0u;
            std::uint32_t and_mask = 0u;
            if ((inv_tgt & 0x30u) != 0u) {
                if (!dp.push_qd(0x1500000018uLL, 56))
                    return false;
                dp.patch_mask(0x200000u, 0xFEDFFFFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                pair_qword = kDmPair14614;
                or_bits = 0x400000u;
                and_mask = 0xFDC000FFu;
            } else {
                if (!dp.push_qd(0x400000018uLL, 54))
                    return false;
                dp.patch_mask(0x10u, 0xFEFFFFEFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                pair_qword = kDmPair14085;
                or_bits = 32u;
                and_mask = 0xFE00000Fu;
            }
            if (!auro_downmix_v1_plan_apply_label29(dp, 25, pair_qword, or_bits, and_mask, src_mask))
                return false;
            if ((tgt_layout & 0x800000u) != 0u)
                goto label_36;
        } else if ((tgt_layout & 0x800000u) != 0u) {
            goto label_36;
        }
        if ((src_mask & 0x800000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(0x1500000017uLL, 59))
                return false;
            dp.patch_mask(0x200000u, 0xFF5FFFFFu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label35(
                    dp, r0, 23, kDmPair15382, 0x400000u, 0xFF40000Fu, src_mask))
                return false;
        }
    }

label_36:
    // only when tgt lacks 0x30000000 and src has it.
    if ((tgt_layout & 0x30000000u) != 0x30000000u && (src_mask & 0x30000000u) == 0x30000000u) {
        if ((inv_tgt & 0x600u) != 0u) {
            std::uint32_t v49 = 2u * static_cast<std::uint32_t>((inv_tgt & 0x30u) != 0u);
            if (eng3 != 0)
                v49 = static_cast<std::uint32_t>(eng3);
            if (v49 == 2u) {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(28u, 12))
                    return false;
                dp.patch_mask(1u, 0xEFFFFFFEu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair3329, 2u, 0xDFFFFFFFu, src_mask))
                    return false;
            } else if (v49 != 0u) {
                return false;
            } else {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(28u, 8))
                    return false;
                dp.patch_mask(1u, 0xEFFFFFFEu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x10000001DuLL, 9))
                    return false;
                dp.patch_mask(2u, 0xDFFFFFFDu);
                if (r0 > 0x31u)
                    return false;
                if (!dp.push_qd(0x40000001CuLL, 10))
                    return false;
                dp.patch_mask(0x10u, 0xEFFFFFEFu);
                if (r0 == 49u)
                    return false;
                *dp.row_count = r0 + 4u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair2821, 32u, 0xDFFFFFC1u, src_mask))
                    return false;
            }
        } else {
            std::uint32_t v31 =
                static_cast<std::uint32_t>((static_cast<std::int32_t>(tgt_layout << 19) >> 31) & 3);
            if (eng2 != 3)
                v31 = static_cast<std::uint32_t>(eng2);
            std::uint32_t v32 = 2u * static_cast<std::uint32_t>((inv_tgt & 0x6000u) != 0u);
            if (v31 != 0u)
                v32 = v31;
            if (v32 == 3u) {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(0xC0000001CuLL, 0))
                    return false;
                dp.patch_mask(0x1000u, 0xEFFFEFFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair268, 4096u, 0xDFFEFBFFu, src_mask))
                    return false;
            } else if (v32 == 2u) {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(0x90000001CuLL, 6))
                    return false;
                dp.patch_mask(0x200u, 0xEFFFFDFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair1802, 1024u, 0xDFFFF7FFu, src_mask))
                    return false;
            } else if (v32 != 0u) {
                return false;
            } else {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(0x90000001CuLL, 2))
                    return false;
                dp.patch_mask(0x200u, 0xEFFFFDFFu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0xA0000001DuLL, 3))
                    return false;
                dp.patch_mask(0x400u, 0xDFFFFBFFu);
                if (r0 > 0x31u)
                    return false;
                if (!dp.push_qd(0xD0000001CuLL, 4))
                    return false;
                dp.patch_mask(0x2000u, 0xEFFFDFFFu);
                if (r0 == 49u)
                    return false;
                *dp.row_count = r0 + 4u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair1294, 0x4000u, 0xDFFFBFFFu, src_mask))
                    return false;
            }
        }
    }

    // Native v200: (tgt_lo!=3) & ((uint8_t)tgt >> 2) — low byte only.
    const std::uint8_t tgt_class_hint =
        (tgt_layout & 3u) != 3u
            ? static_cast<std::uint8_t>(static_cast<std::uint8_t>(tgt_layout) >> 2)
            : static_cast<std::uint8_t>(0);
    // if (tgt&0x1000) || !(src&0x1000) goto; else
    // eng0/eng1 Top routing then fall into.
    // Previous port called first; label209's finish set
    // engine_complete and skipped Top (bit 12) routing for 0x11bf→0x3.
    const bool skip_top_routing =
        (tgt_layout & 0x1000u) != 0u || (src_mask & 0x1000u) == 0u;
    if (!skip_top_routing) {
        bool routed_to_label101 = false;
        if ((tgt_layout & 0x30000000u) == 0x30000000u || (inv_tgt & 0x600u) == 0u) {
            if (!
auro_downmix_v1_plan_height_post_label101(
                    plan, engine, src_mask, tgt_layout, inv_tgt, &routed_to_label101))
                return false;
            // When inv_tgt&0x600 != 0 and tgt lacks 0x30000000, post returns
            // without routing (fall through to eng1 in the native CFG).
            if (!routed_to_label101 && (inv_tgt & 0x600u) != 0u) {
                if (!
auro_downmix_v1_plan_height_tail_eng1(
                        plan, engine, src_mask, tgt_layout, &routed_to_label101))
                    return false;
            }
        } else {
            if (!
auro_downmix_v1_plan_height_tail_eng1(
                    plan, engine, src_mask, tgt_layout, &routed_to_label101))
                return false;
        }
        if (!routed_to_label101)
            return false;
    }

    if (!
auro_downmix_v1_plan_label101(
            plan, engine, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete))
        return false;
    if (engine_complete && *engine_complete)
        return true;

    return
auro_downmix_v1_plan_finalize_label275(plan, tgt_layout, tgt_class_hint);
}

bool
auro_downmix_v1_plan_add_height_routes(std::int32_t* plan, std::uint32_t tgt_layout) {
    auto* row_count = reinterpret_cast<std::uint64_t*>(plan);
    auto* out_mask = reinterpret_cast<std::uint32_t*>(plan + 159);
    const std::uint32_t inv_tgt = ~tgt_layout;
    std::uint32_t mask_word = *out_mask;
    bool result = false;

    if ((~mask_word & 0xC000000u) != 0u || (inv_tgt & 0xC000000u) == 0u) {
        result = true;
        if ((inv_tgt & 0xC0000u) == 0u)
            return result;
    } else {
        std::uint64_t rows = *row_count;
        result = false;
        if ((inv_tgt & 0x30u) != 0u) {
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 26u;
            plan[off + 2] = 63;
            *out_mask = (*out_mask & 0xFBFFFFFEu) | 1u;
            if (rows == 51u)
                return false;
            *row_count = rows + 2u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
            *reinterpret_cast<std::int32_t*>(row_base) = 27;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x4000000001uLL;
            mask_word = (2u | (*out_mask & 0xF7FFFFFFu));
            *out_mask = mask_word;
            result = true;
            if ((inv_tgt & 0xC0000u) == 0u)
                return result;
        } else {
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 26u;
            plan[off + 2] = 65;
            *out_mask = (*out_mask & 0xFBFFFFFEu) | 1u;
            if (rows == 51u)
                return false;
            *row_count = rows + 2u;
            const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
            *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x10000001BLL;
            plan[off1 + 2] = 66;
            *out_mask = (*out_mask & 0xF7FFFFFDu) | 2u;
            if (rows > 0x31u)
                return false;
            *row_count = rows + 3u;
            const std::size_t off2 = 2u + 3u * static_cast<std::size_t>(rows + 2u);
            *reinterpret_cast<std::uint64_t*>(plan + off2) = 0x40000001ALL;
            plan[off2 + 2] = 67;
            *out_mask = (*out_mask & 0xFBFFFFEFu) | 0x10u;
            if (rows == 49u)
                return false;
            *row_count = rows + 4u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 36u;
            *reinterpret_cast<std::int32_t*>(row_base) = 27;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x4400000005uLL;
            mask_word = (32u | (*out_mask & 0xF7FFFFDFu));
            *out_mask = mask_word;
            result = true;
            if ((inv_tgt & 0xC0000u) == 0u)
                return result;
        }
    }

    if ((mask_word & 0xC0000u) != 0xC0000u)
        return result;

    const std::uint64_t rows = *row_count;
    if ((inv_tgt & 7u) != 0u) {
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
        *reinterpret_cast<std::uint64_t*>(plan + off) = 18u;
        plan[off + 2] = 90;
        *out_mask = (*out_mask & 0xFFFBFFFEu) | 1u;
        if (rows == 51u)
            return false;
        *row_count = rows + 2u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
        *reinterpret_cast<std::int32_t*>(row_base) = 19;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x5B00000001uLL;
        *out_mask = (2u | (*out_mask & 0xFFF7FFFDu));
        return true;
    }

    if (rows > 0x33u)
        return false;
    *row_count = rows + 1u;
    {
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
        *reinterpret_cast<std::uint64_t*>(plan + off) = 18u;
        plan[off + 2] = 92;
    }
    *out_mask = (*out_mask & 0xFFFBFFFEu) | 1u;
    if (rows == 51u)
        return false;
    *row_count = rows + 2u;
    {
        const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
        *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x100000013LL;
        plan[off1 + 2] = 93;
    }
    *out_mask = (*out_mask & 0xFFF7FFFDu) | 2u;
    if (rows > 0x31u)
        return false;
    *row_count = rows + 3u;
    {
        const std::size_t off2 = 2u + 3u * static_cast<std::size_t>(rows + 2u);
        *reinterpret_cast<std::uint64_t*>(plan + off2) = 0x200000012LL;
        plan[off2 + 2] = 94;
    }
    *out_mask = (*out_mask & 0xFFFBFFFBu) | 4u;
    if (rows == 49u)
        return false;
    *row_count = rows + 4u;
    auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 36u;
    *reinterpret_cast<std::int32_t*>(row_base) = 19;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x5F00000002uLL;
    *out_mask = (4u | (*out_mask & 0xFFFBFFFBu));
    return true;
}

bool
auro_downmix_v1_plan_add_center_routes(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint) {
    (void)tgt_class_hint;
    auto* row_count = reinterpret_cast<std::uint64_t*>(plan);
    auto* out_mask = reinterpret_cast<std::uint32_t*>(plan + 159);
    std::uint32_t mask_word = *out_mask;
    const std::uint32_t inv_tgt = ~tgt_layout;

    if ((inv_tgt & 3u) != 0u) {
        if ((mask_word & 0x100000u) != 0u && (tgt_layout & 0x100000u) == 0u) {
            const std::uint64_t rows = *row_count;
            if (tgt_class_hint == 0u) {
                if (rows >= 0x34u)
                    return false;
                *row_count = rows + 1u;
                const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
                *reinterpret_cast<std::uint64_t*>(plan + off) = 0x1E00000014uLL;
                plan[off + 2] = 109;
                const std::uint32_t prev = *out_mask;
                *out_mask = (*out_mask & 0xBFEFFFFFu) | 0x40000000u;
                if ((((prev & 8u) >> 3u) & static_cast<std::uint32_t>((tgt_layout & 8u) == 0u)) == 0u)
                    return true;
                const std::uint64_t rows2 = *row_count;
                if (rows2 > 0x33u)
                    return false;
                *row_count = rows2 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows2 + 8u;
                *reinterpret_cast<std::int32_t*>(row_base) = 3;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6E0000001EuLL;
                *out_mask = (0x40000000u | (*out_mask & 0xBFFFFFF9u));
                return true;
            }
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 0x200000014uLL;
            plan[off + 2] = 109;
            mask_word = (*out_mask & 0xFFEFFFFBu) | 4u;
            *out_mask = mask_word;
        }
        if ((tgt_layout & 8u) != 0u || (mask_word & 8u) == 0u)
            return true;
        if (tgt_class_hint != 0u) {
            const std::uint64_t rows = *row_count;
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
            *reinterpret_cast<std::int32_t*>(row_base) = 3;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6E00000002uLL;
            *out_mask = (4u | (*out_mask & 0xFFFFFFF3u));
            return true;
        }
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
        *reinterpret_cast<std::int32_t*>(row_base) = 3;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6E0000001EuLL;
        *out_mask = (0x40000000u | (*out_mask & 0xBFFFFFF9u));
        return true;
    }

    if ((mask_word & 0x100000u) != 0u) {
        if ((tgt_layout & 0x100000u) != 0u)
            return true;
        const std::uint64_t rows = *row_count;
        if ((tgt_layout & 8u) != 0u) {
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
            *reinterpret_cast<std::int32_t*>(row_base) = 20;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6A00000003uLL;
            *out_mask = (8u | (*out_mask & 0xFFEFFFFFu));
            return true;
        }
        if (rows <= 0x33u) {
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 20u;
            plan[off + 2] = 107;
            *out_mask = (*out_mask & 0xFFEFFFFEu) | 1u;
            if (rows != 51u) {
                *row_count = rows + 2u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
                *reinterpret_cast<std::int32_t*>(row_base) = 3;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6C00000001uLL;
                *out_mask = (2u | (*out_mask & 0xFFFFFFF5u));
                return true;
            }
        }
        return false;
    }

    if (((mask_word & 8u) == 0u) | static_cast<std::uint32_t>((tgt_layout & 8u) >> 3u))
        return true;
    const std::uint64_t rows = *row_count;
    if (rows > 0x33u)
        return false;
    *row_count = rows + 1u;
    const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
    *reinterpret_cast<std::uint64_t*>(plan + off) = 3u;
    plan[off + 2] = 104;
    *out_mask = (*out_mask & 0xFFFFFFF6u) | 1u;
    if (rows == 51u)
        return false;
    *row_count = rows + 2u;
    auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
    *reinterpret_cast<std::int32_t*>(row_base) = 3;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6900000001uLL;
    *out_mask = (2u | (*out_mask & 0xFFFFFFF5u));
    return true;
}

bool
auro_downmix_v1_plan_add_front_routes(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint) {
    auto* row_count = reinterpret_cast<std::uint64_t*>(plan);
    auto* out_mask = reinterpret_cast<std::uint32_t*>(plan + 159);
    const std::uint32_t inv_tgt = ~tgt_layout;

    if ((inv_tgt & 3u) != 0u) {
        std::uint32_t mask_word = *out_mask;
        if ((~mask_word & 3u) != 0u) {
            if (tgt_class_hint != 0u) {
                if ((tgt_layout & 0x40000000u) != 0u || (mask_word & 0x40000000u) == 0u)
                    return true;
                const std::uint64_t rows = *row_count;
                if (rows > 0x33u)
                    return false;
                *row_count = rows + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
                *reinterpret_cast<std::int32_t*>(row_base) = 30;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6500000002uLL;
                *out_mask = (4u | (*out_mask & 0xBFFFFFFBu));
                return true;
            }
        } else {
            const std::uint64_t rows = *row_count;
            if (tgt_class_hint != 0u) {
                if (rows >= 0x34u)
                    return false;
                *row_count = rows + 1u;
                const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
                *reinterpret_cast<std::uint64_t*>(plan + off) = 0x200000000uLL;
                plan[off + 2] = 102;
                *out_mask = (*out_mask & 0xFFFFFFFAu) | 4u;
                if (rows == 51u)
                    return false;
                *row_count = rows + 2u;
                const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
                *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x200000001uLL;
                plan[off1 + 2] = 103;
                mask_word = (*out_mask & 0xFFFFFFF9u) | 4u;
                *out_mask = mask_word;
                if ((tgt_layout & 0x40000000u) != 0u || (mask_word & 0x40000000u) == 0u)
                    return true;
                const std::uint64_t rows2 = *row_count;
                if (rows2 > 0x33u)
                    return false;
                *row_count = rows2 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows2 + 8u;
                *reinterpret_cast<std::int32_t*>(row_base) = 30;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6500000002uLL;
                *out_mask = (4u | (*out_mask & 0xBFFFFFFBu));
                return true;
            }
            if (rows >= 0x34u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 0x1E00000000uLL;
            plan[off + 2] = 102;
            *out_mask = (*out_mask & 0xBFFFFFFEu) | 0x40000000u;
            if (rows == 51u)
                return false;
            *row_count = rows + 2u;
            const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
            *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x1E00000001uLL;
            plan[off1 + 2] = 103;
            mask_word = (*out_mask & 0xBFFFFFFDu) | 0x40000000u;
            *out_mask = mask_word;
        }
        if ((tgt_layout & 4u) != 0u)
            return true;
        if ((mask_word & 4u) == 0u)
            return true;
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
        *reinterpret_cast<std::int32_t*>(row_base) = 2;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x620000001EuLL;
        *out_mask = (0x40000000u | (*out_mask & 0xBFFFFFFBu));
        return true;
    }

    std::uint32_t mask_word = *out_mask;
    if ((tgt_layout & 4u) != 0u) {
        if ((mask_word & 0x40000000u) == 0u || (tgt_layout & 0x40000000u) != 0u)
            return true;
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
        *reinterpret_cast<std::int32_t*>(row_base) = 30;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6500000002uLL;
        *out_mask = (4u | (*out_mask & 0xBFFFFFFBu));
        return true;
    }

    if ((mask_word & 4u) != 0u) {
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
        *reinterpret_cast<std::uint64_t*>(plan + off) = 2u;
        plan[off + 2] = 96;
        *out_mask = (*out_mask & 0xFFFFFFFAu) | 1u;
        if (rows == 51u)
            return false;
        *row_count = rows + 2u;
        const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
        *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x100000002uLL;
        plan[off1 + 2] = 97;
        mask_word = (*out_mask & 0xFFFFFFF9u) | 2u;
        *out_mask = mask_word;
        if ((tgt_layout & 0x40000000u) != 0u)
            return true;
    } else if ((tgt_layout & 0x40000000u) != 0u) {
        return true;
    }

    if ((mask_word & 0x40000000u) == 0u)
        return true;
    const std::uint64_t rows = *row_count;
    if (rows > 0x33u)
        return false;
    *row_count = rows + 1u;
    const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
    *reinterpret_cast<std::uint64_t*>(plan + off) = 30u;
    plan[off + 2] = 99;
    *out_mask = (*out_mask & 0xBFFFFFFEu) | 1u;
    if (rows == 51u)
        return false;
    *row_count = rows + 2u;
    auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
    *reinterpret_cast<std::int32_t*>(row_base) = 30;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6400000001uLL;
    *out_mask = (2u | (*out_mask & 0xBFFFFFFDu));
    return true;
}

bool
auro_downmix_v1_plan_finalize_label275(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint) {
    return
auro_downmix_v1_plan_add_height_routes(plan, tgt_layout)
        &&
auro_downmix_v1_plan_add_center_routes(plan, tgt_layout, tgt_class_hint)
        &&
auro_downmix_v1_plan_add_front_routes(plan, tgt_layout, tgt_class_hint);
}

} // namespace

void
auro_downmix_v1_engine_t_construct(std::uint8_t* engine_base) {
    if (!engine_base)
        return;
    std::memset(engine_base, 0, 56u);
}

bool
auro_downmix_v1_engine_calculate(
    std::int32_t* engine,
    std::int32_t* plan,
    std::uint32_t src_layout,
    std::uint32_t tgt_layout) {
    if (!plan)
        return false;
    std::memset(plan, 0, 0x280u);
    // auro_downmix_v1_Engine_calculate early exits.
    if (tgt_layout == 0u || src_layout == 0u) {
        plan[158] = static_cast<std::int32_t>(src_layout);
        plan[159] = 0;
        return true;
    }
    if ((~tgt_layout & src_layout) == 0u) {
        plan[158] = static_cast<std::int32_t>(src_layout);
        plan[159] = static_cast<std::int32_t>(src_layout);
        return true;
    }
    const std::uint32_t tgt_lo = tgt_layout & 3u;
    // Height entry: v200 || tgt&0x40000000 || tgt_lo==3.
    // v200 = (tgt_lo!=3) & ((uint8_t)tgt >> 2). Non-matching layouts return
    // false — Hex-Rays (result=0, cond) has no separate successful branch.
    const std::uint8_t tgt_class_hint =
        tgt_lo != 3u
            ? static_cast<std::uint8_t>(static_cast<std::uint8_t>(tgt_layout) >> 2)
            : static_cast<std::uint8_t>(0);
    const bool height_entry =
        tgt_class_hint != 0u || (tgt_layout & 0x40000000u) != 0u || tgt_lo == 3u;
    if (height_entry) {
        std::uint32_t src_dim_layout = src_layout & 0xBFFFFFFBu | 0x40000000u;
        if ((src_layout & 4u) == 0u)
            src_dim_layout = src_layout;
        if ((~src_layout & 3u) == 0u)
            src_dim_layout = src_layout;
        std::uint32_t tgt_dim_layout = tgt_layout & 0xBFFFFFFBu | 0x40000000u;
        if ((tgt_layout & 4u) == 0u)
            tgt_dim_layout = tgt_layout;
        if (tgt_lo == 3u)
            tgt_dim_layout = tgt_layout;
        (void)auro_channel_Layout_dimension(src_dim_layout);
        (void)auro_channel_Layout_dimension(tgt_dim_layout);
        plan[158] = static_cast<std::int32_t>(src_layout);
        plan[159] = static_cast<std::int32_t>(src_layout);
        bool engine_complete = false;
        if (!
auro_downmix_v1_plan_height_preprocess(
                plan, engine, src_layout, tgt_layout, &engine_complete))
            return false;
        return static_cast<std::uint32_t>(plan[159]) != 0u;
    }
    return false;
}

DownmixToPartial
codec_v3_downmix_to(std::uint32_t layout, std::uint32_t target_layout) {
    DownmixToPartial out{};
    alignas(16) std::uint8_t engine[56]{};
    alignas(16) std::int32_t plan[kDownmixPlanIntCount]{};

auro_downmix_v1_engine_t_construct(engine);
    if (!
auro_downmix_v1_engine_calculate(
            reinterpret_cast<std::int32_t*>(engine),
            plan,
            layout,
            target_layout)) {
        return out;
    }
    const std::uint32_t mask = static_cast<std::uint32_t>(plan[159]);
    if (mask == 0u)
        return out;
    out.mask = mask;
    out.ok = true;
    return out;
}

void codec_v3_layout_pair_from_ctx(std::uint64_t ctx, std::uint32_t* output_layout, std::uint32_t* target_layout) {
    if (output_layout)
        *output_layout = 0u;
    if (target_layout)
        *target_layout = 0u;
    const std::uint64_t codec = codec_v3_codec_decoder_from_ctx(ctx);
    if (codec == 0u)
        return;
    const auto* cfg = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(codec));
    // Match ACV3Decoder:decide_decode_ (a1+88=input/carrier, a1+92=output):
    // fill_started uses downmix(output_layout, target_layout) as the baseline and
    // downmix(frame_flags, target_layout) for the frame — so baseline first arg is
    // Config input_mask, shared second arg is Config output_mask.
    if (output_layout) {
        *output_layout = *reinterpret_cast<const std::uint32_t*>(
            cfg + auro_codec_v3::kDecoderConfig_off_input_mask);
    }
    if (target_layout) {
        *target_layout = *reinterpret_cast<const std::uint32_t*>(
            cfg + auro_codec_v3::kDecoderConfig_off_output_mask);
    }
}

void codec_v3_write_decide_decode_aggregate(std::uint64_t ctx, std::uint32_t aggregate_mask) {
    if (ctx == 0u)
        return;
    auto* blob = reinterpret_cast<std::uint8_t*>(ctx);

    // / auro_a3deng_v3 wrapper: state +2496, notify via +2552.
    if (*reinterpret_cast<const std::uint64_t*>(blob + 2552u) != 0u) {
        std::uint32_t* state_word = reinterpret_cast<std::uint32_t*>(blob + 2496u);
        if (*state_word != aggregate_mask) {
            *state_word = aggregate_mask;
            auro_a3deng_v3_set_decode_state_and_notify(static_cast<std::int64_t>(ctx), 1);
        }
        return;
    }

    // dispatch blob: state +252112, last_result +252128.
    constexpr std::uintptr_t kEb5a0OffDecideDecodeState = 252112u;
    constexpr std::uintptr_t kEb5a0OffLastResult = 252128u;
    std::uint32_t* state_word = reinterpret_cast<std::uint32_t*>(blob + kEb5a0OffDecideDecodeState);
    if (*state_word == aggregate_mask)
        return;
    *state_word = aggregate_mask;
    *reinterpret_cast<std::uint32_t*>(blob + kEb5a0OffLastResult) = 1u;
    auto* sink_holder = reinterpret_cast<std::uint64_t*>(blob + 31521ull * sizeof(std::uint64_t));
    if (!sink_holder || *sink_holder == 0u)
        return;
    const std::uint64_t fn_ptr = *reinterpret_cast<const std::uint64_t*>(*sink_holder + 8u);
    if (fn_ptr == 0u)
        return;
    using NotifyFn = void (__fastcall *)(std::uint64_t, std::int64_t);
    reinterpret_cast<NotifyFn>(fn_ptr)(*sink_holder, 1);
}

} // namespace

std::uint32_t codec_v3_block_info_segment_count_from_decoder(const std::uint8_t* decoder_base) {
    if (!decoder_base)
        return 0u;
    const auto* og = decoder_base + auro_codec_v3::kDecoder_off_OutputGenerator;
    const std::uint64_t seg_ctx = *reinterpret_cast<const std::uint64_t*>(og + kOgOff_segment_ctx_ptr);
    if (seg_ctx == 0u)
        return 0u;
    return *reinterpret_cast<const std::uint32_t*>(seg_ctx + kSegCtxOff_count);
}

void codec_v3_layout_pair_from_decoder(
    const std::uint8_t* decoder_base,
    std::uint32_t* output_layout,
    std::uint32_t* target_layout) {
    if (output_layout)
        *output_layout = 0u;
    if (target_layout)
        *target_layout = 0u;
    if (!decoder_base)
        return;
    // Same pairing as codec_v3_layout_pair_from_ctx ACV3Decoder:decide_decode_.
    if (output_layout) {
        *output_layout = *reinterpret_cast<const std::uint32_t*>(
            decoder_base + auro_codec_v3::kDecoderConfig_off_input_mask);
    }
    if (target_layout) {
        *target_layout = *reinterpret_cast<const std::uint32_t*>(
            decoder_base + auro_codec_v3::kDecoderConfig_off_output_mask);
    }
}

namespace {

std::uint32_t
codec_v3_decide_decode_fill_started_flags(
    std::uint32_t count,
    std::uint64_t ranges_base,
    std::uint64_t started_base,
    std::uint32_t output_layout,
    std::uint32_t target_layout) {
    const DownmixToPartial output_downmix =
codec_v3_downmix_to(output_layout, target_layout);
    if (!output_downmix.ok) {
        for (std::uint32_t i = 0; i < count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        return 0u;
    }

    const std::uint32_t output_count = auro_channel_Mask_count(output_downmix.mask, 0, 0);
    std::uint32_t aggregate_mask = 0u;

    for (std::uint32_t i = 0; i < count; ++i) {
        auto* started_word = reinterpret_cast<std::uint32_t*>(
            started_base + 4ull * static_cast<std::uint64_t>(i));
        const auto* slot = reinterpret_cast<const std::uint8_t*>(
            ranges_base + static_cast<std::uintptr_t>(kSegmentStrideBytes) * static_cast<std::uintptr_t>(i));
        const std::uint32_t seg_flags =
            *reinterpret_cast<const std::uint32_t*>(slot + kSegRangeOff_flags);
        if (*started_word == 0u || seg_flags == 0u) {
            *started_word = 0u;
            continue;
        }

        const DownmixToPartial frame_downmix =
codec_v3_downmix_to(seg_flags, target_layout);
        if (!frame_downmix.ok) {
            *started_word = 0u;
            continue;
        }

        const std::uint32_t frame_count = auro_channel_Mask_count(frame_downmix.mask, 0, 0);
        const bool prefer_started_decode = frame_count > output_count;
        *started_word = prefer_started_decode ? 1u : 0u;
        if (prefer_started_decode)
            aggregate_mask |= seg_flags;
    }
    return aggregate_mask;
}

} // namespace

std::int64_t codec_v3_pre_segments_decide_decode(
    std::uint64_t ctx,
    std::uint64_t ranges_base,
    std::uint64_t started_base) {
    if (started_base == 0u)
        return 0;
    const std::uint32_t count = codec_v3_block_info_segment_count_from_ctx(ctx);
    if (count == 0u)
        return 0;

    if (codec_v3_allow_decoding_from_ctx(ctx) == 0u) {
        for (std::uint32_t i = 0; i < count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        codec_v3_write_decide_decode_aggregate(ctx, 0u);
        return 0;
    }

    std::uint32_t output_layout = 0u;
    std::uint32_t target_layout = 0u;
    codec_v3_layout_pair_from_ctx(ctx, &output_layout, &target_layout);
    const std::uint32_t aggregate_mask =
codec_v3_decide_decode_fill_started_flags(
        count,
        ranges_base,
        started_base,
        output_layout,
        target_layout);
    codec_v3_write_decide_decode_aggregate(ctx, aggregate_mask);
    return 0;
}

std::int64_t codec_v3_pre_segments_decide_decode_decoder(
    std::uint64_t decoder_base,
    std::uint64_t ranges_base,
    std::uint64_t started_base) {
    if (decoder_base == 0u || started_base == 0u)
        return 0;
    const auto* dec = reinterpret_cast<const std::uint8_t*>(decoder_base);
    const std::uint32_t count = codec_v3_block_info_segment_count_from_decoder(dec);
    if (count == 0u)
        return 0;

    if (codec_v3_allow_decoding_from_decoder(dec) == 0u) {
        for (std::uint32_t i = 0; i < count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        codec_v3_write_decide_decode_aggregate(decoder_base, 0u);
        return 0;
    }

    std::uint32_t output_layout = 0u;
    std::uint32_t target_layout = 0u;
    codec_v3_layout_pair_from_decoder(dec, &output_layout, &target_layout);
    const std::uint32_t aggregate_mask =
codec_v3_decide_decode_fill_started_flags(
        count,
        ranges_base,
        started_base,
        output_layout,
        target_layout);
    codec_v3_write_decide_decode_aggregate(
        static_cast<std::uint64_t>(decoder_base), aggregate_mask);
    return 0;
}

std::int64_t codec_v3_pre_segments_decide_decode_layouts(
    std::uint32_t segment_count,
    std::uint64_t ranges_base,
    std::uint64_t started_base,
    std::uint32_t output_layout,
    std::uint32_t target_layout,
    std::uint32_t allow_decoding,
    CodecV3DispatchStateEb5a0* aggregate_dispatch) {
    if (started_base == 0u || segment_count == 0u)
        return 0;

    if (allow_decoding == 0u) {
        for (std::uint32_t i = 0; i < segment_count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        if (aggregate_dispatch) {
            if (aggregate_dispatch->decide_decode_state != 0u) {
                aggregate_dispatch->decide_decode_state = 0u;
                aggregate_dispatch->last_result = 1u;
                notify_codec_v3_state_change(aggregate_dispatch, 1);
            } else {
                aggregate_dispatch->last_result = 0u;
            }
        }
        return 0;
    }

    const std::uint32_t aggregate_mask =
codec_v3_decide_decode_fill_started_flags(
        segment_count,
        ranges_base,
        started_base,
        output_layout,
        target_layout);
    if (aggregate_dispatch) {
        if (aggregate_dispatch->decide_decode_state != aggregate_mask) {
            aggregate_dispatch->decide_decode_state = aggregate_mask;
            aggregate_dispatch->last_result = 1u;
            notify_codec_v3_state_change(aggregate_dispatch, 1);
        } else {
            aggregate_dispatch->last_result = 0u;
        }
    }
    return 0;
}

void codec_v3_decide_decode_callback_eb8a0(
    CodecV3DispatchStateEb5a0* state,
    std::uint32_t next_state,
    std::uint32_t* decisions,
    int decision_count) {
    if (!state)
        return;
    const std::uint32_t prev_decode_state = state->decide_decode_state;
    std::uint32_t current_decode_state = 0u;

    if (decision_count <= 0) {
        state->last_result = 0u;
        if (prev_decode_state == 0u)
            return;
    } else {
        bool unchanged = true;
        for (int i = 0; i != decision_count; ++i) {
            current_decode_state = 0u;
            if (decisions && decisions[i] != 0u) {
                const bool gate_enabled = state->decide_decode_gate != 0u;
                decisions[i] = gate_enabled ? 1u : 0u;
                if (gate_enabled)
                    current_decode_state = next_state;
            } else if (decisions) {
                decisions[i] = 0u;
            }
            unchanged = unchanged && (current_decode_state == prev_decode_state);
        }
        state->last_result = unchanged ? 0u : 1u;
        if (prev_decode_state == current_decode_state)
            return;
    }

    state->decide_decode_state = current_decode_state;
    notify_codec_v3_state_change(state, 1);
}

std::int64_t codec_v3_dispatch(
    CodecV3DispatchStateEb5a0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status,
    CodecV3ProcessFnEb5a0 process,
    void* process_user) {
    CodecV3IoBufferDescEb5a0 in_local{};
    CodecV3IoBufferDescEb5a0 out_local{};
    const std::uint32_t total_samples = state->block_size != 0u ? state->block_size : 64u;
    in_local.total_samples = total_samples;
    in_local.sample_rate = state->sample_rate;
    in_local.bits_per_sample = 24u;
    out_local.total_samples = total_samples;
    out_local.sample_rate = state->sample_rate;
    out_local.bits_per_sample = 24u;

    for (std::size_t i = 0; i < kCodecV3ChannelCount; ++i) {
        in_local.channel_ptr[i] = input_desc->channel_ptr[i];
        out_local.channel_ptr[i] = output_desc->channel_ptr[i];
    }

    const std::int64_t result =
        process(process_user, &in_local, state->format_word0, &out_local, io_status);
    if (*io_status == 0u)
        *io_status = state->format_word0;
    state->last_result = *io_status;
    return result;
}

std::int64_t codec_v3_process_partial_eb5a0(
    void* user,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t format_word0,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status) {
    auto* runtime = reinterpret_cast<CodecV3PartialRuntimeEb5a0*>(user);

    const std::uint32_t input_mask = format_word0 & kCodecV3ChannelMask;
    runtime->dispatch->format_word0 = input_mask;
    runtime->dispatch->sample_rate = input_desc->sample_rate;

    if (delay_line_write_buffer_106ab0(runtime->delay_line, input_desc, input_mask) != 0) {
        format_detector_process(
            runtime->format_detector,
            input_desc,
            input_mask,
            runtime->set_layout,
            runtime->process_block,
            runtime->sync_user);
        // FormatDetector owns +308; propagate to dispatch (not the reverse).
        if (runtime->dispatch)
            runtime->dispatch->sync_state = runtime->format_detector->sync_state;
    }

    *io_status = input_mask;
    return 0;
}

std::int32_t codec_v3_decoder_process_validate_101800(
    const std::uint8_t* decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    const CodecV3IoBufferDescEb5a0* output_desc) {
    // current Decoder_process:
    // 3 null input/output, 401 input total_samples mismatch, 386 sample_rate mismatch,
    // 387 bits mismatch, 388 missing input ptr, 389 missing output ptr, 384 disallowed input mask.
    if (!input_desc || !output_desc)
        return 3;
    if (!decoder_base)
        return 401;

    const std::uint64_t expected_total_samples =
        *reinterpret_cast<const std::uint64_t*>(decoder_base + 0u);
    const std::uint32_t expected_sample_rate =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + 32u);
    const std::uint32_t required_output_mask =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + 40u);
    const std::uint32_t allowed_input_mask =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + 36u);

    if (input_desc->total_samples != expected_total_samples)
        return 401;
    if (input_desc->sample_rate != expected_sample_rate)
        return 386;
    if (input_desc->bits_per_sample != 24u)
        return 387;

    if (output_desc->total_samples != input_desc->total_samples)
        return 401;
    if (output_desc->sample_rate != input_desc->sample_rate)
        return 386;
    if (output_desc->bits_per_sample != 24u)
        return 387;

    const std::uint32_t in_mask31 = input_mask & kCodecV3ChannelMask;
    if (!codec_v3_has_required_channel_ptrs_101800(input_desc, in_mask31))
        return 388;
    if (!codec_v3_has_required_channel_ptrs_101800(output_desc, required_output_mask))
        return 389;

    if ((in_mask31 & allowed_input_mask) != in_mask31)
        return 384;
    return 0;
}

std::int64_t codec_v3_decoder_process_101800(
    std::uint8_t* decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status,
    const void* output_runtime_fns,
    CodecV3ParserProcessFn101800 parser_process) {
    std::int64_t result = 3;
    if (!input_desc)
        return result;
    if (!output_desc)
        return result;

    const std::uint64_t v9 = input_desc->total_samples;
    result = 401;
    if (!decoder_base || *reinterpret_cast<const std::uint64_t*>(decoder_base + 0u) != v9)
        return result;

    const std::uint32_t v10 = input_desc->sample_rate;
    if (v10 != *reinterpret_cast<const std::uint32_t*>(decoder_base + 32u))
        return 386;
    if (input_desc->bits_per_sample != 24u)
        return 387;
    if (static_cast<std::uint32_t>(v9) != output_desc->total_samples)
        return result;

    result = 386;
    if (output_desc->sample_rate != v10)
        return result;
    result = 387;
    if (output_desc->bits_per_sample != 24u)
        return result;

    std::uint64_t v12 = 0u;
    std::uint32_t v13 = 1u;
    if ((input_mask & 1u) != 0u) {
        if (input_desc->channel_ptr[0] == 0u)
            return 388;
    }
    while (true) {
        const std::uint32_t out_mask = *reinterpret_cast<const std::uint32_t*>(decoder_base + 40u);
        if ((v13 & out_mask) != 0u && output_desc->channel_ptr[v12] == 0u)
            return 389;
        ++v12;
        if (v12 == kCodecV3ChannelCount)
            break;
        v13 = (1u << v12);
        if (((input_mask >> v12) & 1u) != 0u) {
            if (input_desc->channel_ptr[v12] == 0u)
                return 388;
        }
    }

    result = 384;
    if ((input_mask & *reinterpret_cast<const std::uint32_t*>(decoder_base + 36u)) != input_mask)
        return result;

    auto* delay_line_base = decoder_base + auro_codec_v3::kDecoder_off_Memory;
    (void)delay_line_write_buffer_106ab0(
        reinterpret_cast<DelayLineState106b40*>(delay_line_base),
        input_desc,
        input_mask);
    format_detector_process(
        &reinterpret_cast<FormatDetectorRegion1056c0*>(
            decoder_base + auro_codec_v3::kDecoder_off_FormatDetector)->tail,
        input_desc,
        input_mask,
        format_detector_integrated_set_layout,
        format_detector_integrated_process_block,
        decoder_base + auro_codec_v3::kDecoder_off_FormatDetector);
    (void)(parser_process ? parser_process : codec_v3_parser_process_integrated_1034e0)(
        decoder_base + auro_codec_v3::kDecoder_off_Parser);

    std::uint64_t v15 = 0u;
    while (true) {
        if (((*reinterpret_cast<const std::uint32_t*>(decoder_base + 40u) >> v15) & 1u) != 0u) {
            const std::uint64_t v17 = *reinterpret_cast<const std::uint64_t*>(decoder_base + 0u);
            if (v17 != 0u) {
                auto* dst = reinterpret_cast<std::int32_t*>(output_desc->channel_ptr[v15]);
                if (dst) {
                    for (std::uint64_t i = 0; i < v17; ++i)
                        dst[i] = 0;
                }
            }
        }
        ++v15;
        if (v15 == kCodecV3ChannelCount)
            break;
    }

    OutputGeneratorRuntimeFns1024a9 fns = codec_v3_default_output_runtime_1024a9();
    const std::uint64_t og_pre_cb = *reinterpret_cast<const std::uint64_t*>(
        decoder_base + auro_codec_v3::kDecoder_off_OutputGenerator + kOgOff_pre_segments_cb);
    if (og_pre_cb == 0u) {
        fns.pre_segments_callback = codec_v3_pre_segments_decide_decode_decoder;
        fns.pre_segments_ctx = reinterpret_cast<std::uint64_t>(decoder_base);
    }
    if (output_runtime_fns) {
        const auto& ext = *reinterpret_cast<const OutputGeneratorRuntimeFns1024a9*>(output_runtime_fns);
        if (ext.delay_line_get_channel) fns.delay_line_get_channel = ext.delay_line_get_channel;
        if (ext.delay_line_get_buffer) fns.delay_line_get_buffer = ext.delay_line_get_buffer;
        if (ext.frame_deque_find_first_with_end_after)
            fns.frame_deque_find_first_with_end_after = ext.frame_deque_find_first_with_end_after;
        if (ext.pre_segments_callback) {
            fns.pre_segments_callback = ext.pre_segments_callback;
            fns.pre_segments_ctx = ext.pre_segments_ctx;
        }
        if (ext.metadata_update_callback) {
            fns.metadata_update_callback = ext.metadata_update_callback;
            fns.metadata_update_ctx = ext.metadata_update_ctx;
        }
        if (ext.decode_channel_segment) fns.decode_channel_segment = ext.decode_channel_segment;
        if (ext.golombrice_get_errors) fns.golombrice_get_errors = ext.golombrice_get_errors;
        if (ext.extrapolate_process) fns.extrapolate_process = ext.extrapolate_process;
        if (ext.golombrice_initialize) fns.golombrice_initialize = ext.golombrice_initialize;
        if (ext.extrapolate_initialize) fns.extrapolate_initialize = ext.extrapolate_initialize;
        if (ext.frame_mark_as_unused) fns.frame_mark_as_unused = ext.frame_mark_as_unused;
        if (ext.frame_deque_pop_front) fns.frame_deque_pop_front = ext.frame_deque_pop_front;
    }

    std::uint32_t local_status = 0u;
    std::uint32_t* status_ptr = io_status ? io_status : &local_status;
    (void)output_generator_process(
        decoder_base + auro_codec_v3::kDecoder_off_OutputGenerator,
        reinterpret_cast<std::uint64_t>(output_desc),
        fns,
        status_ptr,
        input_desc,
        input_mask);
    (void)delay_line_advance_106b20(reinterpret_cast<DelayLineState106b40*>(delay_line_base));
    return 0;
}

std::int64_t auro_codec_v3_Decoder_process(
    std::uint64_t decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status) {
    return codec_v3_decoder_process_101800(
        reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(decoder_base)),
        input_desc,
        input_mask,
        output_desc,
        io_status,
        nullptr,
        nullptr);
}

// auro_codec_v3:_codec_Parser_process.
std::int64_t codec_v3_parser_process_1034e0(
    std::uint8_t* parser_base,
    const CodecV3ParserRuntimeFns1034e0* runtime_fns) {
    if (!parser_base || !runtime_fns
        || !runtime_fns->delay_line_get_buffer
        || !runtime_fns->frame_deque_find_first_with_end_after
        || !runtime_fns->frame_deque_push_back
        || !runtime_fns->frame_deque_pop_front
        || !runtime_fns->frame_mark_as_unused) {
        return 0;
    }

    auto* a1 = reinterpret_cast<std::uint64_t*>(parser_base);
    std::uint64_t v27 = 0;
    std::uint64_t v31 = runtime_fns->delay_line_get_buffer(a1[2], a1[1], &v27);
    std::uint64_t result = v31;
    std::uint64_t v3 = v27;
    std::uint64_t v4 = a1[3];
    if (v4 <= v27)
        return static_cast<std::int64_t>(result);

    auto* v30 = reinterpret_cast<std::uint32_t*>(a1 + 7);
    auto* v29 = a1;

    while (true) {
        const std::uint64_t v5 = v4 - v3;
        std::uint64_t v6 = runtime_fns->frame_deque_find_first_with_end_after(a1[4], a1[1]);
        std::uint64_t v9 = 0;
        if (v6 == 0) {
            v9 = v5;
            a1[1] += v5;
            if (*reinterpret_cast<std::uint32_t*>(parser_base + 648u) != 0u) {
                *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = 0u;
                if (runtime_fns->content_state_callback)
                    runtime_fns->content_state_callback(runtime_fns->content_state_ctx, 0u);
            }
        } else {
            const std::uint64_t frame_start = *reinterpret_cast<const std::uint64_t*>(v6 + 0u);
            const std::uint64_t cursor = a1[1];
            const std::uint64_t v8 = frame_start - cursor;
            const std::uint32_t frame_start_delta32 =
                static_cast<std::uint32_t>(frame_start) - static_cast<std::uint32_t>(cursor);
            if (static_cast<std::int32_t>(frame_start_delta32) > 0) {
                v9 = (v5 < v8) ? v5 : v8;
                a1[1] += v9;
                if (*reinterpret_cast<std::uint32_t*>(parser_base + 648u) != 0u) {
                    *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = 0u;
                    if (runtime_fns->content_state_callback)
                        runtime_fns->content_state_callback(runtime_fns->content_state_ctx, 0u);
                }
            } else {
                v9 = *reinterpret_cast<const std::uint64_t*>(v6 + 8u) - cursor;
                if (v5 < v9)
                    v9 = v5;
                const std::uint64_t v32 = v27;
                std::uint32_t v10 = *reinterpret_cast<const std::uint32_t*>(v6 + 44u);

                // Native frames already own ParseResult objects when parsing
                // starts in the middle of a host block. Host-reconstructed
                // frames can enter here with a negative start delta and null
                // per-channel ParseResult pointers, so bind only missing slots.
                if (v10 != 0u) {
                    std::uint64_t v11 = v6 + 56u;
                    std::uint64_t v12 = reinterpret_cast<std::uint64_t>(v30);
                    std::uint64_t v13 = 0;
                    do {
                        auto* parse_result_slot = reinterpret_cast<std::uint64_t*>(v11 + 16u);
                        if (*parse_result_slot == 0u) {
                            const std::uint64_t v14 =
                                ::auro3deng::parse_result_pool_get_new(a1[6]);
                            *parse_result_slot = v14;
                            ::auro3deng::channel_parser_construct(
                                v12,
                                v14,
                                v6 + 28u,
                                v11);
                        }
                        ++v13;
                        v10 = *reinterpret_cast<const std::uint32_t*>(v6 + 44u);
                        v11 += 32u;
                        v12 += 64u;
                    } while (v13 < v10);
                }

                std::int32_t v18 = 0;
                if (v10 != 0u) {
                    const std::uint64_t v33 = 32ull * static_cast<std::uint64_t>(v10);
                    std::uint64_t v15 = 0;
                    std::uint32_t* v17 = v30;
                    std::int32_t v16 = 1;
                    std::int32_t v19 = 1;

                    while (true) {
                        if (*reinterpret_cast<const std::uint32_t*>(v6 + 52u + v15) != 0u) {
                            const std::uint32_t ch = *reinterpret_cast<const std::uint32_t*>(v6 + 48u + v15);
                            const std::uint64_t frame_channel_ptr =
                                *reinterpret_cast<const std::uint64_t*>(v6 + 72u + v15);
                            const std::uint64_t channel = delay_line_get_channel_from_buffer_106ab0(v31, ch, v32);
                            std::uint32_t mode = 0;
                            const bool ok = ::auro3deng::channel_parser_process_103840(
                                                reinterpret_cast<std::uint64_t>(v17),
                                                frame_channel_ptr,
                                                channel,
                                                static_cast<std::uint32_t>(v9),
                                                &mode)
                                != 0;
                            const std::int32_t v22 = *reinterpret_cast<const std::int32_t*>(frame_channel_ptr + 92u);
                            if (v18 == 0)
                                v18 = v22;
                            const bool v23 = (v22 == 0);
                            const bool v24 = (v22 == v18);
                            v16 = (v16 != 0 && ok && (v24 || v23)) ? 1 : 0;
                            v19 = (v24 && v19 != 0) ? 1 : 0;
                        }
                        v15 += 32u;
                        v17 += 16u;
                        if (v15 == v33)
                            break;
                    }

                    if (v16 == 0) {
                        runtime_fns->frame_mark_as_unused(v6);
                        result = runtime_fns->frame_deque_pop_front(a1[4]);
                        a1[1] += v9;
                        if (*reinterpret_cast<std::uint32_t*>(parser_base + 648u) != 0u) {
                            *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = 0u;
                            if (runtime_fns->content_state_callback)
                                runtime_fns->content_state_callback(runtime_fns->content_state_ctx, 0u);
                        }
                        v3 = v27 + v9;
                        v27 = v3;
                        v4 = a1[3];
                        if (v4 <= v3)
                            return static_cast<std::int64_t>(result);
                        continue;
                    }

                    if (v19 != 0)
                        *reinterpret_cast<std::int32_t*>(v6 + 24u) = v18;
                } else {
                    *reinterpret_cast<std::int32_t*>(v6 + 24u) = 0;
                }

                a1[1] += v9;
                const std::uint32_t v25 = *reinterpret_cast<const std::uint32_t*>(v6 + 24u);
                if (v25 != 0u && v25 != *reinterpret_cast<std::uint32_t*>(parser_base + 648u)) {
                    *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = v25;
                    if (runtime_fns->content_state_callback)
                        runtime_fns->content_state_callback(runtime_fns->content_state_ctx, v25);
                }
                if (*reinterpret_cast<const std::uint32_t*>(v6 + 8u) == static_cast<std::uint32_t>(a1[1])) {
                    result = runtime_fns->frame_deque_push_back(a1[5], v6);
                    result = runtime_fns->frame_deque_pop_front(a1[4]);
                }
            }
        }

        v3 = v27 + v9;
        v27 = v3;
        v4 = a1[3];
        if (v4 <= v3)
            return static_cast<std::int64_t>(result);
    }
}

std::int32_t codec_v3_decoder_process_validate(
    const CodecV3DispatchStateEb5a0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    const CodecV3IoBufferDescEb5a0* output_desc) {
    if (!input_desc || !output_desc)
        return 3;
    if (!state)
        return 401;
    const std::uint32_t expected_total_samples = state->block_size != 0u ? state->block_size : 64u;
    if (input_desc->total_samples != expected_total_samples)
        return 401;
    if (input_desc->sample_rate != state->sample_rate)
        return 386;
    if (input_desc->bits_per_sample != 24u)
        return 387;
    if (output_desc->total_samples != input_desc->total_samples)
        return 401;
    if (output_desc->sample_rate != input_desc->sample_rate)
        return 386;
    if (output_desc->bits_per_sample != 24u)
        return 387;
    if (!codec_v3_has_required_channel_ptrs_101800(input_desc, input_mask & kCodecV3ChannelMask))
        return 388;
    const std::uint32_t required_output_mask =
        state->required_output_mask != 0u ? state->required_output_mask : (input_mask & kCodecV3ChannelMask);
    if (!codec_v3_has_required_channel_ptrs_101800(output_desc, required_output_mask))
        return 389;
    if (((~state->format_word0 & input_mask) & kCodecV3ChannelMask) != 0u)
        return 384;
    return 0;
}

std::uint64_t delay_line_get_buffer_106b40(
    DelayLineState106b40* state,
    std::uint64_t cursor,
    std::int64_t* io_state) {
    if (io_state)
        *io_state = 0;
    if (!state || state->samples_per_block == 0 || state->ring_slot_count == 0 || state->ring_storage_base == 0)
        return 0;

    const std::uint32_t delta32 =
        static_cast<std::uint32_t>(state->absolute_cursor) - static_cast<std::uint32_t>(cursor);
    if (static_cast<std::int32_t>(delta32) <= 0)
        return 0;
    const std::uint64_t delta = delta32;

    const std::uint64_t block_distance =
        (delta - 1ull) / static_cast<std::uint64_t>(state->samples_per_block);
    const std::uint32_t needed_blocks = static_cast<std::uint32_t>(block_distance + 1ull);
    if (needed_blocks >= state->ring_slot_count)
        return 0;

    if (io_state) {
        *io_state = static_cast<std::int64_t>(
            delta - static_cast<std::uint64_t>(needed_blocks) * static_cast<std::uint64_t>(state->samples_per_block));
    }

    std::uint32_t wrap_base = 0;
    if (needed_blocks > state->write_slot_index)
        wrap_base = state->ring_slot_count;
    const std::uint32_t slot = wrap_base + state->write_slot_index - needed_blocks;
    return state->ring_storage_base + kDelayLineBufferSlotStrideBytes * slot;
}

std::uint64_t delay_line_get_buffer_u64_state(
    std::uint64_t delay_line_ptr,
    std::uint64_t cursor,
    std::uint64_t* io_state_u64) {
    std::int64_t io_state = 0;
    const std::uint64_t out = delay_line_get_buffer_106b40(
        reinterpret_cast<DelayLineState106b40*>(delay_line_ptr),
        cursor,
        &io_state);
    if (io_state_u64)
        *io_state_u64 = static_cast<std::uint64_t>(io_state);
    return out;
}

std::uint64_t delay_line_get_channel_from_buffer_106ab0(
    std::uint64_t delay_line_buffer,
    std::uint32_t channel,
    std::uint64_t start) {
    if (delay_line_buffer == 0 || channel >= kCodecV3ChannelCount)
        return 0;
    const auto* slot = reinterpret_cast<const DelayLineBufferSlot106b40*>(delay_line_buffer);
    const std::uint64_t base = slot->channel_ptr[channel];
    return base ? (base + 4ull * start) : 0;
}

std::uint64_t delay_line_stream_index(
    const DelayLineState106b40* state,
    std::uint32_t stage_count) {
    if (!state)
        return 0;
    return state->absolute_cursor
        - static_cast<std::uint64_t>(state->samples_per_block)
            * static_cast<std::uint64_t>(stage_count);
}

std::uint64_t delay_line_advance_106b20(DelayLineState106b40* state) {
    if (!state || state->ring_slot_count == 0)
        return 0;
    const std::uint32_t current = state->write_slot_index;
    state->absolute_cursor += state->samples_per_block;
    const std::uint32_t next = (current + 1u) % state->ring_slot_count;
    const std::uint64_t wrapped = (current + 1u) / state->ring_slot_count;
    state->write_slot_index = next;
    return wrapped;
}

std::uint64_t delay_line_write_buffer_106ab0(
    DelayLineState106b40* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    if (!state || !input_desc || state->ring_storage_base == 0 || state->samples_per_block == 0)
        return 0;
    auto* slot = reinterpret_cast<DelayLineBufferSlot106b40*>(
        state->ring_storage_base + kDelayLineBufferSlotStrideBytes * state->write_slot_index);
    slot->channel_mask = input_mask;

    for (std::uint32_t bit = 0; bit < kCodecV3ChannelCount; ++bit) {
        if (((input_mask >> bit) & 1u) == 0)
            continue;
        const std::uint64_t src = input_desc->channel_ptr[bit];
        const std::uint64_t dst = slot->channel_ptr[bit];
        if (!src || !dst)
            continue;
        for (std::uint32_t i = 0; i < state->samples_per_block; ++i) {
            *reinterpret_cast<std::int32_t*>(dst + 4ull * i) =
                *reinterpret_cast<const std::int32_t*>(src + 4ull * i);
        }
    }
    return reinterpret_cast<std::uint64_t>(slot);
}

// Mirrors auro_codec_v3_decoder_FormatDetector_process.
void sync_detector_t_construct(SyncDetectorState105ee0* state) {
    if (!state)
        return;
    std::memset(state, 0, sizeof(*state));
}

void sync_detector_set_callback(
    SyncDetectorState105ee0* state,
    void (*notify)(void* ctx, std::int64_t kind, std::uint64_t a, std::uint64_t b),
    void* notify_ctx) {
    if (!state)
        return;
    state->notify = notify;
    state->notify_ctx = notify_ctx;
}

const std::uint32_t* sync_detector_get_common_header(const SyncDetectorState105ee0* state) {
    if (!state || state->enabled == 0u || state->state != 1u)
        return nullptr;
    return &state->detect_counter;
}

const SyncDetectorChannelState105ee0* sync_detector_find_channel_header(
    const SyncDetectorState105ee0* state,
    std::uint32_t channel) {
    if (!state || state->enabled == 0u || state->state != 1u)
        return nullptr;
    const std::uint32_t n = std::min<std::uint32_t>(
        state->active_channel_count,
        static_cast<std::uint32_t>(std::size(state->channels)));
    for (std::uint32_t i = 0; i < n; ++i) {
        if (state->channels[i].channel == channel)
            return &state->channels[i];
    }
    return nullptr;
}

void format_detector_t_construct(
    FormatDetectorRegion1056c0* region,
    std::uint8_t* decoder_base,
    std::uint8_t* memory_base) {
    if (!region || !decoder_base || !memory_base)
        return;
    std::memset(region, 0, sizeof(*region));
    region->tail.frame_deque_ptr =
        reinterpret_cast<std::uint64_t>(memory_base + 40u);
    region->tail.blocks_per_call = *reinterpret_cast<const std::uint32_t*>(
        decoder_base + auro_codec_v3::kDecoderConfig_off_block_words);
    region->tail.allow_low_9bits = *reinterpret_cast<const std::uint32_t*>(
        decoder_base + auro_codec_v3::kDecoderConfig_off_extra_flags);
    const auto* delay_line = reinterpret_cast<const DelayLineState106b40*>(memory_base);
    region->tail.processed_samples =
        delay_line_stream_index(delay_line, 0u);
    sync_detector_t_construct(&region->sync);
    sync_detector_set_callback(
        &region->sync,
        format_detector_sync_callback,
        region);
}

void format_detector_sync_callback(
    void* region_raw,
    std::int64_t kind,
    std::uint64_t rel_start,
    std::uint64_t span) {
    auto* region = reinterpret_cast<FormatDetectorRegion1056c0*>(region_raw);
    if (!region)
        return;
    auto& tail = region->tail;

    if (kind == 2) {
        if (tail.frame_deque_ptr != 0u) {
            (void)frame_deque_pop_back(
                tail.frame_deque_ptr,
                frame_mark_as_unused_106cd0_default);
        }
        // kind==2: clear sync_state only (not expected_frame_end +328).
        if (tail.sync_state != 0u) {
            tail.sync_state = 0;
            if (tail.sink_notify)
                tail.sink_notify(tail.sink_user, 0);
        }
        return;
    }

    if (kind != 0)
        return;

    const std::int64_t rel = static_cast<std::int64_t>(static_cast<std::int32_t>(rel_start));
    const std::uint64_t frame_start = tail.processed_samples + static_cast<std::uint64_t>(rel);
    const std::uint32_t frame_span = static_cast<std::uint32_t>(span != 0u ? span : kCodecV3FrameDequeSlotCopyBytes);
    const std::uint32_t layout_word = tail.layout;
    const std::uint32_t sub_52ced0_and =
        (tail.allow_low_9bits != 0u)
            ? static_cast<std::uint32_t>(auro_codec_v3::kFormatDetector52ced0_frame_mask_and_when_allow_nonzero)
            : static_cast<std::uint32_t>(static_cast<std::int32_t>(
                auro_codec_v3::kFormatDetector52ced0_frame_mask_and_when_allow_zero));
    const std::uint32_t active_mask = layout_word & sub_52ced0_and;

    // Frame_t_construct always, then push only when common_header is valid.
    std::array<std::uint8_t, kCodecV3FrameDequeSlotCopyBytes> frame{};
    frame_construct(
        reinterpret_cast<std::uint64_t>(frame.data()),
        frame_start,
        frame_span,
        layout_word,
        active_mask);

    const std::uint32_t* common_header =
        sync_detector_get_common_header(&region->sync);
    if (!common_header)
        return;
    std::memcpy(frame.data() + 28u, common_header, 16u);

    const std::uint32_t slot_count =
        *reinterpret_cast<const std::uint32_t*>(frame.data() + 44u);
    for (std::uint32_t si = 0; si < slot_count; ++si) {
        auto* slot = frame.data() + kCodecV3FrameOffSlotBase
            + static_cast<std::size_t>(si) * kCodecV3FrameSlotStride;
        if (*reinterpret_cast<const std::uint32_t*>(slot + 4u) == 0u)
            continue;
        const std::uint32_t ch = *reinterpret_cast<const std::uint32_t*>(slot + 0u);
        const auto* header = sync_detector_find_channel_header(&region->sync, ch);
        if (!header)
            return;
        // get_channel_header returns &aligned_word; qword +0 -> slot+8, dword +8 -> slot+16.
        *reinterpret_cast<std::uint64_t*>(slot + 8u) =
            (static_cast<std::uint64_t>(header->aligned_bit) << 32)
            | static_cast<std::uint64_t>(header->aligned_word);
        *reinterpret_cast<std::uint32_t*>(slot + 16u) = header->aligned_code;
    }

    if (tail.frame_deque_ptr == 0u)
        return;
    if (frame_deque_push_back(
            tail.frame_deque_ptr,
            reinterpret_cast<std::uint64_t>(frame.data()),
            kCodecV3FrameDequeSlotCopyBytes) == 0)
        return;

    tail.expected_frame_end = frame_start + static_cast<std::uint64_t>(frame_span);
    if (tail.sync_state == 0u) {
        tail.sync_state = 1u;
        if (tail.sink_notify)
            tail.sink_notify(tail.sink_user, 1);
    }
}

void format_detector_process(
    FormatDetectorState1056c0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    void (*set_layout)(void* user, std::uint32_t layout_mask),
    void (*process_block)(void* user, const std::uint64_t* channel_ptrs_27),
    void* sync_user) {
    if (!state || !input_desc)
        return;

    if (state->layout != input_mask) {
        // FormatDetector_process: clear sync_state only (not expected_frame_end +328).
        if (state->sync_state != 0) {
            state->sync_state = 0;
            if (state->sink_notify)
                state->sink_notify(state->sink_user, 0);
        }
        state->layout = input_mask;
        const std::uint32_t v6 = (state->allow_low_9bits != 0u)
            ? static_cast<std::uint32_t>(auro_codec_v3::kFormatDetector52ced0_frame_mask_and_when_allow_nonzero)
            : static_cast<std::uint32_t>(static_cast<std::int32_t>(
                auro_codec_v3::kFormatDetector52ced0_frame_mask_and_when_allow_zero));
        const std::uint32_t layout_mask = (input_mask & kCodecV3ChannelMask) & v6;
        if (set_layout)
            set_layout(sync_user, layout_mask);
    }

    if (state->blocks_per_call == 0)
        return;

    // Prefer the real host sample count so unit-block 1000 is fully scanned
    // (32*floor(1000/32) would drop the last 8 LSB samples).
    const std::uint64_t sample_span = input_desc->total_samples != 0u
        ? static_cast<std::uint64_t>(input_desc->total_samples)
        : 32ull * static_cast<std::uint64_t>(state->blocks_per_call);
    for (std::uint64_t sample_off = 0; sample_off < sample_span; ) {
        const std::uint32_t chunk = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(32ull, sample_span - sample_off));
        std::uint64_t block_ptrs[kCodecV3ChannelCount]{};
        for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
            if (((state->layout >> ch) & 1u) == 0)
                continue;
            const std::uint64_t base = input_desc->channel_ptr[ch];
            if (!base)
                continue;
            block_ptrs[ch] = base + 4ull * sample_off;
        }

        if (chunk == 32u) {
            if (process_block)
                process_block(sync_user, block_ptrs);
        } else if (sync_user) {
            sync_detector_process_block(
                reinterpret_cast<SyncDetectorState105ee0*>(sync_user),
                block_ptrs,
                chunk);
        }
        state->processed_samples += chunk;
        sample_off += chunk;
        // timeout unlock clears sync_state only; expected_frame_end stays until next push.
        if (state->sync_state != 0u) {
            const auto remaining = static_cast<std::int64_t>(
                state->expected_frame_end - state->processed_samples);
            if (remaining <= -17) {
                state->sync_state = 0;
                if (state->sink_notify)
                    state->sink_notify(state->sink_user, 0);
            }
        }
    }
}

// Mirrors auro_codec_v3_decoder_SyncDetector_set_layout.
void sync_detector_set_layout(SyncDetectorState105ee0* state, std::uint32_t layout_mask) {
    if (!state || state->layout == layout_mask)
        return;

    if (state->state == 1u && state->notify)
        state->notify(state->notify_ctx, 2, 0, 0);

    state->state = 0;
    state->counter_4 = 0;
    state->accum_8 = 0;
    state->accum_c = 0;
    state->word_10 = 0;
    state->reserved_14 = 0;
    state->active_channel_count = 0;
    state->detect_counter = 0;
    state->detect_bit_110 = 0;
    state->detect_bit_114 = 0;
    state->detect_target = 0;
    for (auto& ch : state->channels)
        ch = {};

    state->layout = layout_mask;
    state->enabled = (layout_mask < 0x200u) ? 1u : 0u;
    if (!state->enabled)
        return;

    static constexpr std::uint32_t kOrder[] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u};
    std::uint32_t count = 0;
    for (std::uint32_t channel : kOrder) {
        if (((layout_mask >> channel) & 1u) == 0)
            continue;
        if (count >= std::size(state->channels))
            break;
        state->channels[count].channel = channel;
        ++count;
    }
    state->active_channel_count = count;
}

// Mirrors auro_codec_v3_decoder_SyncDetector_process_block.
void sync_detector_process_block(
    SyncDetectorState105ee0* state,
    const std::uint64_t* channel_ptrs_27,
    std::uint32_t sample_count) {
    if (!state || !channel_ptrs_27 || state->enabled == 0)
        return;
    if (sample_count == 0u)
        return;
    if (sample_count > 32u)
        sample_count = 32u;

    auto merge_sync_window16 = [](std::uint32_t old_hist, std::uint32_t new_hist, std::uint32_t sample_idx) {
        const std::uint32_t carry_bits = (sample_idx <= 15u) ? (16u - sample_idx) : 0u;
        const std::uint32_t new_bits = 16u - carry_bits;
        const std::uint32_t old_mask = (carry_bits == 0u) ? 0u : ((1u << carry_bits) - 1u);
        const std::uint32_t new_mask = (new_bits >= 32u) ? 0xFFFFFFFFu : ((1u << new_bits) - 1u);
        const std::uint32_t old_part = (old_hist & old_mask) << new_bits;
        const std::uint32_t new_part =
            (new_bits == 0u) ? 0u : ((new_hist >> (32u - sample_idx)) & new_mask);
        return old_part | new_part;
    };

    std::uint32_t any_lsb[32]{};
    std::uint32_t any_bit2[32]{};
    std::uint32_t hist_bit1[9]{};
    std::uint32_t hist_bit2[9]{};

    const std::uint32_t active_count = std::min<std::uint32_t>(
        state->active_channel_count,
        static_cast<std::uint32_t>(std::size(state->channels)));

    for (std::uint32_t i = 0; i < active_count; ++i) {
        const std::uint32_t channel = state->channels[i].channel;
        const std::uint64_t src = channel < kCodecV3ChannelCount ? channel_ptrs_27[channel] : 0;
        if (!src)
            continue;

        std::uint32_t packed_bit1 = 0;
        std::uint32_t packed_bit2 = 0;
        for (std::uint32_t j = 0; j < sample_count; ++j) {
            const std::uint32_t sample =
                static_cast<std::uint32_t>(*reinterpret_cast<const std::int32_t*>(src + 4ull * j));
            const std::uint32_t sample_lsb = (sample & 1u) + 1u;
            const std::uint32_t sample_bit2 = ((sample & 4u) != 0u) ? 2u : 1u;
            any_lsb[j] |= sample_lsb;
            any_bit2[j] |= sample_bit2;
            packed_bit1 = ((sample >> 1) & 1u) + (packed_bit1 << 1);
            packed_bit2 = ((sample & 4u) >> 2) + (packed_bit2 << 1);
        }
        // Keep history packing left-aligned in the 32-bit word like a full block.
        if (sample_count < 32u) {
            packed_bit1 <<= (32u - sample_count);
            packed_bit2 <<= (32u - sample_count);
        }
        hist_bit1[i] = packed_bit1;
        hist_bit2[i] = packed_bit2;
    }

    for (std::uint32_t j = 0; j < sample_count; ++j) {
        const std::uint32_t lsb_flags = any_lsb[j];
        const std::uint32_t bit2_flags = any_bit2[j];

        bool keep_locked = (state->state == 1u);
        if (state->state == 0u) {
            if (lsb_flags == 1u && state->counter_4 >= 16u) {
                const std::uint32_t accum_8 = state->accum_8;
                std::uint32_t detect_target = (((accum_8 >> 4) & 0xFF0u) + 0x10u);
                if (detect_target == 0x3E0u)
                    detect_target = 0x3E8u;

                const bool preamble_ok = (static_cast<std::int8_t>(accum_8 & 0xFFu) >= 0) &&
                    ((state->accum_c & 0xFF00u) == 0xFF00u) &&
                    (detect_target >= 0x100u && detect_target <= 0x400u) &&
                    ((state->accum_c & 0xB0u) == 0xB0u) &&
                    (((accum_8 >> 5) & 1u) != 0u);
                if (preamble_ok) {
                    for (std::uint32_t i = 0; i < active_count; ++i) {
                        auto& ch = state->channels[i];
                        const std::uint32_t merged_bit1 = merge_sync_window16(ch.history_bit1, hist_bit1[i], j);
                        const std::uint32_t merged_bit2 = merge_sync_window16(ch.history_bit2, hist_bit2[i], j);
                        ch.aligned_word = merged_bit1 & 0xFFFFu;
                        ch.aligned_bit = (merged_bit2 >> 6) & 1u;
                        ch.aligned_code = (merged_bit2 & 0xFu) + 10u;
                    }

                    // lock: qword +268 = 0x100000000 → counter=0, bit_110=1;
                    // dword +276 = (accum_8>>4)&1; dword +280 = detect_target.
                    state->detect_counter = 0;
                    state->detect_bit_110 = 1u;
                    state->detect_bit_114 = (accum_8 >> 4u) & 1u;
                    state->detect_target = detect_target;
                    state->word_10 = 16u;
                    state->state = 1u;
                    keep_locked = true;

                    if (state->notify) {
                        const std::int64_t start_pos = static_cast<std::int64_t>(static_cast<std::int32_t>(j)) - 16ll;
                        state->notify(
                            state->notify_ctx,
                            0,
                            static_cast<std::uint64_t>(start_pos),
                            static_cast<std::uint64_t>(detect_target));
                    }
                }
            }
        } else if (state->state == 1u) {
            if (lsb_flags != 1u && (state->word_10 & 0xFu) == 0u && state->word_10 < state->detect_target) {
                if (state->notify)
                    state->notify(state->notify_ctx, 2, j, 0);
                state->state = 0;
                keep_locked = false;
            }
        }

        state->counter_4 = (lsb_flags == 2u) ? (state->counter_4 + 1u) : 0u;
        state->accum_8 = ((state->accum_8 & 0x7FFFu) << 1) | ((bit2_flags == 2u) ? 1u : 0u);
        state->accum_c = ((state->accum_c & 0x7FFFu) << 1) | ((bit2_flags != 3u) ? 1u : 0u);
        ++state->word_10;

        if (keep_locked && state->state == 1u && state->word_10 == state->detect_target) {
            state->state = 0;
            state->counter_4 = 0;
            if (state->notify)
                state->notify(state->notify_ctx, 1, static_cast<std::uint64_t>(j) + 1u, 0);
        }
    }

    for (std::uint32_t i = 0; i < active_count; ++i) {
        state->channels[i].history_bit1 = hist_bit1[i];
        state->channels[i].history_bit2 = hist_bit2[i];
    }
}

OutputGeneratorSegmentPlan output_generator_build_segment_plan_1024a9(
    std::uint8_t* output_generator_base,
    std::uint32_t timeline_cursor,
    std::uint32_t* io_channel_mask_out) {
    OutputGeneratorSegmentPlan plan{};
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    auto* base_q = reinterpret_cast<std::uint64_t*>(output_generator_base);
    auto* seg_ctx = reinterpret_cast<std::uint8_t*>(base_q[kOgOff_segment_ctx_ptr / 8]);

    std::int64_t dl_state = 0;
    plan.delay_line_buffer = delay_line_get_buffer_106b40(
        reinterpret_cast<DelayLineState106b40*>(base_q[kOgOff_delay_line_ptr / 8]),
        timeline_cursor,
        &dl_state);
    plan.delay_line_state_offset = dl_state;
    auto* buffer = reinterpret_cast<std::uint32_t*>(plan.delay_line_buffer);
    if (!buffer)
        return plan;

    *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = 0;
    std::uint64_t consumed = 0;
    const std::uint64_t total = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples);

    std::uint64_t guard = 0;
    while (consumed < total && guard++ < kMaxSegmentBuildGuard) {
        const std::uint32_t seg_idx = *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count);
        const std::uint64_t seg_start = consumed;
        std::uint64_t seg_len = total - consumed;
        std::uint64_t frame_ptr = 0;
        bool frame_has_started = false;
        std::uint32_t frame_flags = 0;

        const std::uint64_t abs_pos = timeline_cursor + consumed;
        frame_ptr = frame_deque_find_first_with_end_after(base_q[kOgOff_frame_deque_ptr / 8], abs_pos);

        if (frame_ptr != 0) {
            const std::uint64_t frame_start_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 0);
            const std::int32_t abs_pos32 = static_cast<std::int32_t>(abs_pos);
            frame_has_started = (static_cast<std::int32_t>(frame_start_u64) - abs_pos32 <= 0);
            if (!frame_has_started) {
                const std::uint64_t delta_to_start = frame_start_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_start);
            }
            if (frame_has_started) {
                // seg_len = min(remaining, frame_end - abs_pos); pop happens in process.
                const std::uint64_t frame_end_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 8);
                const std::uint64_t delta_to_end = frame_end_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_end);
                frame_flags = *reinterpret_cast<const std::uint32_t*>(frame_ptr + 24);
            }
        }
        // Запись как в v10 + 24*idx: start/len/mask/flags.
        const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
        auto* slot = reinterpret_cast<std::uint8_t*>(ranges_base + kSegmentStrideBytes * seg_idx);
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_start) = seg_start;
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_len) = seg_len;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_mask) = *buffer;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_flags) = frame_flags;

        // store frame_ptr only when timeline-started; prestart keeps nullptr.
        const std::uint64_t stored_frame_ptr = frame_has_started ? frame_ptr : 0u;
        const std::uint64_t frame_ptrs_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_ptrs);
        *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uint8_t*>(frame_ptrs_base) + 8ull * seg_idx) = stored_frame_ptr;
        const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
        *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(started_base) + 4ull * seg_idx) = frame_has_started ? 1u : 0u;

        OutputGeneratorSegment item{};
        item.start = seg_start;
        item.len = seg_len;
        item.channel_mask = *buffer;
        item.frame_flags = frame_flags;
        item.frame_ptr = stored_frame_ptr;
        item.frame_has_started = frame_has_started;
        item.prefer_started_decode_path = frame_has_started;
        plan.segments.push_back(item);
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(item);

        consumed += seg_len;
        *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = seg_idx + 1;

        if (seg_len == 0)
            break;
    }

    plan.segment_count = static_cast<std::uint32_t>(plan.segments.size());
    return plan;
}

std::int64_t output_generator_apply_segments_1024a9(
    const OutputGeneratorSegmentPlan& plan,
    const OutputGeneratorApplyCallbacks& cb,
    std::uint32_t* io_channel_mask_out) {
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    std::int64_t rc = 0;
    for (const auto& seg : plan.segments) {
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(seg);
        if (seg.len == 0) {
            rc = warmup_zero_len_channels_1024a9(seg, cb);
            if (rc != 0)
                return rc;
            continue;
        }

        for (int ch = next_set_channel_bit_1024a9(seg.channel_mask, 0);
             ch >= 0;
             ch = next_set_channel_bit_1024a9(seg.channel_mask, ch + 1)) {
            const auto channel = static_cast<std::uint32_t>(ch);
            const std::uint64_t src = cb.get_delay_line_channel(cb.user, channel, seg.start);
            const std::uint64_t out_base = cb.get_output_channel_base(cb.user, channel);
            const std::uint64_t dst = out_base + 4ull * segment_output_start_1024a9(seg, cb);

            if (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg) && cb.decode_channel_segment) {
                rc = cb.decode_channel_segment(cb.user, &seg, channel, src, dst, seg.len, nullptr);
                if (rc != 0)
                    return rc;
            } else {
                copy_i32_samples(dst, src, seg.len);
            }
        }
    }
    return rc;
}

std::int64_t output_generator_prepare_frame_channels_1024a9(
    const OutputGeneratorSegment& seg,
    std::uint64_t timeline_cursor,
    const OutputGeneratorFrameInitCallbacks& cb) {
    if (seg.frame_ptr == 0 || !should_prepare_frame_init_1024a9(seg.frame_ptr, timeline_cursor))
        return 0;

    const std::uint32_t channel_count = cb.get_frame_channel_count(cb.user, seg.frame_ptr);
    for (std::uint32_t i = 0; i < channel_count; ++i) {
        const std::uint64_t frame_ch = cb.get_frame_channel_ptr(cb.user, seg.frame_ptr, i);
        const std::uint64_t gr_state = cb.get_golombrice_state_ptr(cb.user, i);
        const std::uint64_t ex_state = cb.get_extrapolate_state_ptr(cb.user, i);
        std::uint32_t* words_ptr = cb.get_channel_words_ptr(cb.user, frame_ch);
        const std::uint64_t ctx_ptr = cb.get_channel_ctx_ptr(cb.user, frame_ch);

        // both init calls are fire-and-forget, return values are ignored.
        (void)cb.golombrice_initialize(cb.user, gr_state, words_ptr, ctx_ptr);
        (void)cb.extrapolate_initialize(cb.user, ex_state, frame_ch);
    }
    return 0;
}

std::int64_t output_generator_process_segments_1024a9(
    const OutputGeneratorSegmentPlan& plan,
    std::uint64_t timeline_cursor_at_entry,
    const OutputGeneratorFrameInitCallbacks* frame_init_cb,
    const OutputGeneratorApplyCallbacks& apply_cb,
    std::uint32_t* io_channel_mask_out) {
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    std::uint64_t timeline_cursor = timeline_cursor_at_entry;
    for (const auto& seg : plan.segments) {
        if (frame_init_cb && seg.frame_ptr != 0
            && should_prepare_frame_init_1024a9(seg.frame_ptr, timeline_cursor)) {
            const std::int64_t rc_init =
                output_generator_prepare_frame_channels_1024a9(seg, timeline_cursor, *frame_init_cb);
            if (rc_init != 0)
                return rc_init;
        }
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(seg);
        if (seg.len == 0) {
            const std::int64_t rc_warmup = warmup_zero_len_channels_1024a9(seg, apply_cb);
            if (rc_warmup != 0)
                return rc_warmup;
            continue;
        }

        for (int ch = next_set_channel_bit_1024a9(seg.channel_mask, 0);
             ch >= 0;
             ch = next_set_channel_bit_1024a9(seg.channel_mask, ch + 1)) {
            const auto channel = static_cast<std::uint32_t>(ch);
            const std::uint64_t src = apply_cb.get_delay_line_channel(apply_cb.user, channel, seg.start);
            const std::uint64_t out_base = apply_cb.get_output_channel_base(apply_cb.user, channel);
            const std::uint64_t dst = out_base + 4ull * segment_output_start_1024a9(seg, apply_cb);

            if (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg) && apply_cb.decode_channel_segment) {
                const std::int64_t rc_dec =
                    apply_cb.decode_channel_segment(apply_cb.user, &seg, channel, src, dst, seg.len, nullptr);
                if (rc_dec != 0)
                    return rc_dec;
            } else {
                copy_i32_samples(dst, src, seg.len);
            }
        }
        timeline_cursor += seg.len;
    }
    return 0;
}

std::int64_t output_generator_process_segments_raw_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorSegmentPlan& plan,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* io_channel_mask_out,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    OgRawRuntimeCtx ctx{};
    ctx.og = output_generator_base;
    ctx.out_tbl = output_channels_table_base;
    ctx.delay_line_buffer = plan.delay_line_buffer;
    ctx.delay_line_state_offset = plan.delay_line_state_offset;
    ctx.delay_line_ptr = output_generator_base
        ? *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_delay_line_ptr)
        : 0u;
    ctx.total_samples = output_generator_base
        ? *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples)
        : 0u;
    ctx.input_desc = input_desc;
    ctx.input_mask = input_mask & kCodecV3ChannelMask;
    ctx.fns = fns;

    OutputGeneratorFrameInitCallbacks ficb{};
    ficb.user = &ctx;
    ficb.get_frame_channel_count = raw_get_frame_channel_count;
    ficb.get_frame_channel_ptr = raw_get_frame_channel_ptr;
    ficb.get_golombrice_state_ptr = raw_get_gr_state_ptr;
    ficb.get_extrapolate_state_ptr = raw_get_ex_state_ptr;
    ficb.get_channel_words_ptr = raw_get_channel_words_ptr;
    ficb.get_channel_ctx_ptr = raw_get_channel_ctx_ptr;
    ficb.golombrice_initialize = raw_gr_initialize;
    ficb.extrapolate_initialize = raw_ex_initialize;

    OutputGeneratorApplyCallbacks acb{};
    acb.user = &ctx;
    acb.copy_input_enabled =
        output_generator_base
        && *reinterpret_cast<const std::uint32_t*>(output_generator_base + 1232u) != 0u
        && input_desc != nullptr;
    acb.input_mask = ctx.input_mask;
    acb.output_block_start = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor);
    acb.get_delay_line_channel = raw_get_delay_line_channel;
    acb.get_input_channel_base = raw_get_input_channel_base;
    acb.get_output_channel_base = raw_get_output_channel_base;
    acb.decode_channel_segment = raw_decode_channel_segment;

    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    std::uint64_t timeline_cursor = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor);
    const std::uint64_t frame_deque_ptr = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_frame_deque_ptr);
    for (const auto& seg : plan.segments) {
        if (should_apply_frame_metadata_1024a9(seg.frame_ptr, timeline_cursor)) {
            if (should_prepare_frame_init_1024a9(seg.frame_ptr, timeline_cursor)) {
                const std::int64_t rc_init =
                    output_generator_prepare_frame_channels_1024a9(seg, timeline_cursor, ficb);
                if (rc_init != 0)
                    return rc_init;
            }
            output_generator_apply_frame_metadata_1024a9(output_generator_base, seg, timeline_cursor, ficb);
            output_generator_dispatch_metadata_update_1024a9(output_generator_base, fns);
        }

        if (seg.len == 0u) {
            if (!(seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg))) {
                const std::int64_t rc_warmup = warmup_zero_len_channels_1024a9(seg, acb);
                if (rc_warmup != 0)
                    return rc_warmup;
            } else {
                (void)process_segment_started_path_1024a9(
                    output_generator_base, output_channels_table_base, seg, acb, nullptr);
            }
            if (io_channel_mask_out) {
                const std::uint32_t external_mask =
                    (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg))
                        ? frame_output_mask_1024a9(seg)
                        : seg.channel_mask;
                *io_channel_mask_out |= external_mask;
            }
            *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor) = timeline_cursor;
            if (is_frame_finished_at_cursor_1024a9(seg.frame_ptr, timeline_cursor)) {
                if (seg.frame_ptr != 0 && fns.frame_mark_as_unused)
                    fns.frame_mark_as_unused(seg.frame_ptr);
                if (fns.frame_deque_pop_front)
                    fns.frame_deque_pop_front(frame_deque_ptr);
            }
            continue;
        }

        bool segment_used_copy_input = false;
        std::uint32_t segment_produced_mask = 0u;
        if (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg)) {
            const std::int64_t rc_started = process_segment_started_path_1024a9(
                output_generator_base, output_channels_table_base, seg, acb, &segment_produced_mask);
            if (rc_started != 0)
                return rc_started;
        } else if (seg.frame_ptr != 0) {
            const std::int64_t rc_prestart = process_segment_prestart_frame_path_1024a9(
                output_generator_base, seg, acb, &segment_produced_mask);
            if (rc_prestart != 0)
                return rc_prestart;
            segment_used_copy_input = acb.copy_input_enabled;
        } else {
            const std::int64_t rc_copy =
                process_segment_copy_only_path_1024a9(seg, acb, &segment_produced_mask);
            if (rc_copy != 0)
                return rc_copy;
            segment_used_copy_input = acb.copy_input_enabled;
        }
        // og+309 (v107): 0 после copy-input, 1 после decode/delay.
        *reinterpret_cast<std::uint32_t*>(output_generator_base + kOgOff_output_status_flag) =
            segment_used_copy_input ? 0u : 1u;

        const bool started_decode = seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg);
        // Old: started decode jumps directly to;
        // zero-missing only runs on the delay-line/copy path.
        if (!started_decode) {
            zero_unproduced_output_channels_1024a9(
                seg,
                acb,
                segment_produced_mask,
                kCodecV3ChannelMask);
        }

        // 153: started (v131) -> *a3 |= frame+24, без zero-missing.
        // non-started -> *a3 |= *buffer (seg.channel_mask) после copy/warmup.
        if (io_channel_mask_out) {
            const std::uint32_t external_mask =
                started_decode
                    ? frame_output_mask_1024a9(seg)
                    : seg.channel_mask;
            *io_channel_mask_out |= external_mask;
        }
        timeline_cursor += seg.len;
        *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor) = timeline_cursor;

        if (is_frame_finished_at_cursor_1024a9(seg.frame_ptr, timeline_cursor)) {
            if (seg.frame_ptr != 0 && fns.frame_mark_as_unused)
                fns.frame_mark_as_unused(seg.frame_ptr);
            if (fns.frame_deque_pop_front)
                fns.frame_deque_pop_front(frame_deque_ptr);
        }
    }

    return 0;
}

OutputGeneratorSegmentPlan output_generator_build_segment_plan_runtime_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* io_channel_mask_out) {
    OutputGeneratorSegmentPlan plan{};
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;
    if (!output_generator_base || !fns.delay_line_get_buffer || !fns.frame_deque_find_first_with_end_after)
        return plan;

    auto* seg_ctx = reinterpret_cast<std::uint8_t*>(*reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_segment_ctx_ptr));
    if (!seg_ctx)
        return plan;

    std::int64_t dl_state = 0;
    const std::uint64_t delay_line_ptr = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_delay_line_ptr);
    const std::uint64_t timeline_cursor = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor);
    auto* buffer = reinterpret_cast<std::uint32_t*>(fns.delay_line_get_buffer(delay_line_ptr, timeline_cursor, &dl_state));
    if (!buffer)
        return plan;
    plan.delay_line_buffer = reinterpret_cast<std::uint64_t>(buffer);
    plan.delay_line_state_offset = dl_state;

    *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = 0;
    std::uint64_t consumed = 0;
    const std::uint64_t total = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples);
    const std::uint64_t frame_deque_ptr = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_frame_deque_ptr);

    std::uint64_t guard = 0;
    while (consumed < total && guard++ < kMaxSegmentBuildGuard) {
        const std::uint32_t seg_idx = *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count);
        const std::uint64_t seg_start = consumed;
        std::uint64_t seg_len = total - consumed;
        std::uint64_t frame_ptr = 0;
        bool frame_has_started = false;
        std::uint32_t frame_flags = 0;

        const std::uint64_t abs_pos = timeline_cursor + consumed;
        frame_ptr = fns.frame_deque_find_first_with_end_after(frame_deque_ptr, abs_pos);

        if (frame_ptr != 0) {
            const std::uint64_t frame_start_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 0);
            const std::int32_t abs_pos32 = static_cast<std::int32_t>(abs_pos);
            frame_has_started = (static_cast<std::int32_t>(frame_start_u64) - abs_pos32 <= 0);
            if (!frame_has_started) {
                const std::uint64_t delta_to_start = frame_start_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_start);
            }
            if (frame_has_started) {
                // seg_len = min(remaining, frame_end - abs_pos); pop happens in process.
                const std::uint64_t frame_end_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 8);
                const std::uint64_t delta_to_end = frame_end_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_end);
                frame_flags = *reinterpret_cast<const std::uint32_t*>(frame_ptr + 24);
            }
        }
        const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
        auto* slot = reinterpret_cast<std::uint8_t*>(ranges_base + kSegmentStrideBytes * seg_idx);
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_start) = seg_start;
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_len) = seg_len;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_mask) = *buffer;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_flags) = frame_flags;

        // store frame_ptr only when timeline-started; prestart keeps nullptr.
        const std::uint64_t stored_frame_ptr = frame_has_started ? frame_ptr : 0u;
        const std::uint64_t frame_ptrs_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_ptrs);
        *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uint8_t*>(frame_ptrs_base) + 8ull * seg_idx) = stored_frame_ptr;
        const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
        *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(started_base) + 4ull * seg_idx) = frame_has_started ? 1u : 0u;

        OutputGeneratorSegment item{
            seg_start,
            seg_len,
            *buffer,
            frame_flags,
            stored_frame_ptr,
            frame_has_started,
            frame_has_started};
        plan.segments.push_back(item);
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(item);
        consumed += seg_len;
        *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = seg_idx + 1;

        if (seg_len == 0)
            break;
    }

    plan.segment_count = static_cast<std::uint32_t>(plan.segments.size());
    return plan;
}

void output_generator_refresh_segment_plan_from_ctx_1024a9(
    OutputGeneratorSegmentPlan& plan,
    std::uint8_t* seg_ctx) {
    if (!seg_ctx)
        return;
    const std::uint64_t ranges_base = *reinterpret_cast<const std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
    if (ranges_base == 0u)
        return;
    const std::uint64_t frame_ptrs_base = *reinterpret_cast<const std::uint64_t*>(seg_ctx + kSegCtxOff_frame_ptrs);
    const std::uint64_t started_base = *reinterpret_cast<const std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
    std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(seg_ctx + kSegCtxOff_count);
    if (count > plan.segments.size())
        count = static_cast<std::uint32_t>(plan.segments.size());
    plan.segments.resize(count);
    plan.segment_count = count;
    for (std::uint32_t i = 0; i < count; ++i) {
        auto* slot = reinterpret_cast<const std::uint8_t*>(ranges_base + kSegmentStrideBytes * i);
        OutputGeneratorSegment& seg = plan.segments[i];
        seg.start = *reinterpret_cast<const std::uint64_t*>(slot + kSegRangeOff_start);
        seg.len = *reinterpret_cast<const std::uint64_t*>(slot + kSegRangeOff_len);
        seg.channel_mask = *reinterpret_cast<const std::uint32_t*>(slot + kSegRangeOff_mask);
        seg.frame_flags = *reinterpret_cast<const std::uint32_t*>(slot + kSegRangeOff_flags);
        if (frame_ptrs_base != 0u) {
            seg.frame_ptr = *reinterpret_cast<const std::uint64_t*>(
                reinterpret_cast<const std::uint8_t*>(frame_ptrs_base) + 8ull * i);
        }
        if (started_base != 0u) {
            seg.prefer_started_decode_path =
                *reinterpret_cast<const std::uint32_t*>(
                    reinterpret_cast<const std::uint8_t*>(started_base) + 4ull * i) != 0u;
        }
    }
}

std::int64_t output_generator_process_1024a9(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    if (external_mask_inout)
        *external_mask_inout = 0;
    std::int64_t result = 0;

    OutputGeneratorSegmentPlan plan =
        output_generator_build_segment_plan_runtime_1024a9(output_generator_base, fns, nullptr);
    auto* seg_ctx = output_generator_base
        ? reinterpret_cast<std::uint8_t*>(*reinterpret_cast<std::uint64_t*>(
            output_generator_base + kOgOff_segment_ctx_ptr))
        : nullptr;
    if (!output_generator_base)
        return result;
    if (fns.pre_segments_callback) {
        if (seg_ctx) {
            const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
            const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
            result = fns.pre_segments_callback(fns.pre_segments_ctx, ranges_base, started_base);
        }
    } else if (*reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_pre_segments_cb) != 0) {
        if (seg_ctx) {
            const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
            const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
            const auto cb = reinterpret_cast<PreSegmentsFn>(*reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_pre_segments_cb));
            const std::uint64_t ctx = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_pre_segments_ctx);
            result = cb(ctx, ranges_base, started_base);
        }
    }
    if (result != 0)
        return result;
    output_generator_refresh_segment_plan_from_ctx_1024a9(plan, seg_ctx);
    if (plan.segment_count == 0)
        return result;
    return output_generator_process_segments_raw_1024a9(
        output_generator_base,
        plan,
        output_channels_table_base,
        fns,
        external_mask_inout,
        input_desc,
        input_mask);
}

std::int64_t output_generator_process(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    return output_generator_process_1024a9(
        output_generator_base,
        output_channels_table_base,
        fns,
        external_mask_inout,
        input_desc,
        input_mask);
}

std::int64_t decoder_run_output_generator(
    const DecoderOutputGeneratorRunContext1024a9* ctx) {
    std::memset(ctx->output_channels_table, 0, ctx->output_channels_table_size);
    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
        *reinterpret_cast<std::uint64_t*>(ctx->output_channels_table + 16ull + 8ull * ch) =
            ctx->output_channel_ptrs_27[ch];
    }
    CodecV3IoBufferDescEb5a0 input_desc{};
    input_desc.total_samples = ctx->total_samples;
    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
        input_desc.channel_ptr[ch] = ctx->input_channel_ptrs_27
            ? ctx->input_channel_ptrs_27[ch]
            : 0u;
    }

    *reinterpret_cast<std::uint64_t*>(ctx->output_generator_base + kOgOff_delay_line_ptr) = ctx->delay_line_ptr;
    *reinterpret_cast<std::uint64_t*>(ctx->output_generator_base + kOgOff_total_samples) = ctx->total_samples;
    *reinterpret_cast<std::uint64_t*>(ctx->output_generator_base + kOgOff_frame_deque_ptr) = ctx->frame_deque_ptr;

    std::uint32_t output_mask = 0;
    const std::int64_t rc = output_generator_process(
        ctx->output_generator_base,
        reinterpret_cast<std::uint64_t>(ctx->output_channels_table),
        ctx->runtime_fns,
        &output_mask,
        ctx->input_channel_ptrs_27 ? &input_desc : nullptr,
        ctx->input_mask);
    if (ctx->produced_output_mask)
        *ctx->produced_output_mask = output_mask;
    return rc;
}

void output_generator_construct(
    std::uint8_t* output_generator_base,
    const std::uint8_t* output_config,
    std::uint64_t memory_base) {
    if (!output_generator_base || !output_config)
        return;
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_delay_line_ptr) = memory_base;
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_frame_deque_ptr) = memory_base + 72u;
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_segment_ctx_ptr) = memory_base + 104u;
    std::memcpy(output_generator_base + kOgOff_errors_buf, reinterpret_cast<const void*>(memory_base + 152u), 16u);
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples) =
        *reinterpret_cast<const std::uint64_t*>(output_config + 0u);
    *reinterpret_cast<std::uint32_t*>(output_generator_base + 832u) =
        *reinterpret_cast<const std::uint32_t*>(output_config + 8u);
    *reinterpret_cast<std::uint32_t*>(output_generator_base + 836u) =
        *reinterpret_cast<const std::uint32_t*>(output_config + 28u);
    const auto* delay_line = reinterpret_cast<const DelayLineState106b40*>(memory_base);
    const std::uint32_t latency_blocks =
        *reinterpret_cast<const std::uint32_t*>(output_config + 28u);
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor) =
        delay_line_stream_index(delay_line, latency_blocks);
    std::memset(output_generator_base + 888u, 0, 0x158u);
    std::memset(output_generator_base + kOgOff_pre_segments_cb, 0, 32u);
    const std::uint32_t copy_input_flag = *reinterpret_cast<const std::uint32_t*>(output_config + 56u);
    *reinterpret_cast<std::uint32_t*>(output_generator_base + 1232u) = copy_input_flag;
    *reinterpret_cast<std::uint32_t*>(output_generator_base + kOgOff_output_status_flag) =
        copy_input_flag == 0u ? 1u : 0u;
}


