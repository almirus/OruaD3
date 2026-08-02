#include "binaural_renderer.hpp"
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>

namespace auro3d {
namespace {
struct IrHeader { char magic[8]; std::uint32_t version, rate, frames, slots, perf, rooms, banks, mask, slot_ids[10]; };
using C = std::complex<double>;

void fft(std::vector<C>& a, bool inverse) {
    const std::size_t n=a.size();
    for(std::size_t i=1,j=0;i<n;i++){std::size_t b=n>>1;for(;j&b;b>>=1)j^=b;j^=b;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t len=2;len<=n;len<<=1){double ang=2.0*3.14159265358979323846/len*(inverse?-1:1);C wlen(std::cos(ang),std::sin(ang));
        for(std::size_t i=0;i<n;i+=len){C w(1);for(std::size_t j=0;j<len/2;j++){C u=a[i+j],v=a[i+j+len/2]*w;a[i+j]=u+v;a[i+j+len/2]=u-v;w*=wlen;}}}
    if(inverse)for(C& x:a)x/=double(n);
}
double read_sample(const std::uint8_t* p,unsigned bits){
    if(bits==16){std::int16_t v;std::memcpy(&v,p,2);return v/32768.0;}
    if(bits==24){std::int32_t v=(std::int32_t(p[0])|(std::int32_t(p[1])<<8)|(std::int32_t(p[2])<<16));if(v&0x800000)v|=~0xffffff;return v/8388608.0;}
    std::int32_t v;std::memcpy(&v,p,4);return v/2147483648.0;
}
void write_sample(std::vector<std::uint8_t>& out,double x,unsigned bits){
    x=std::max(-1.0,std::min(0.999999999,x));
    if(bits==16){auto v=(std::int16_t)std::llround(x*32767.0);auto*p=(std::uint8_t*)&v;out.insert(out.end(),p,p+2);return;}
    if(bits==24){auto v=(std::int32_t)std::llround(x*8388607.0);out.push_back(v&255);out.push_back((v>>8)&255);out.push_back((v>>16)&255);return;}
    auto v=(std::int32_t)std::llround(x*2147483647.0);auto*p=(std::uint8_t*)&v;out.insert(out.end(),p,p+4);
}
}

struct BinauralStreamRenderer::Impl {
    unsigned bits = 0;
    unsigned channels = 0;
    unsigned bytes_per_sample = 0;
    std::size_t maximum_block_frames = 0;
    std::size_t ir_frames = 0;
    std::size_t nfft = 0;
    std::vector<std::vector<C>> ir_left;
    std::vector<std::vector<C>> ir_right;
    std::vector<double> overlap_left;
    std::vector<double> overlap_right;
};

BinauralStreamRenderer::BinauralStreamRenderer() = default;
BinauralStreamRenderer::~BinauralStreamRenderer() = default;
BinauralStreamRenderer::BinauralStreamRenderer(BinauralStreamRenderer&&) noexcept = default;
BinauralStreamRenderer& BinauralStreamRenderer::operator=(BinauralStreamRenderer&&) noexcept = default;

bool BinauralStreamRenderer::initialize(
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::size_t maximum_block_frames,
    std::string& err) {
    HRSRC rs = FindResourceW(nullptr, MAKEINTRESOURCEW(102), MAKEINTRESOURCEW(10));
    if (!rs) { err = "binaural IR resource not found"; return false; }
    HGLOBAL hg = LoadResource(nullptr, rs);
    const auto* data = static_cast<const std::uint8_t*>(LockResource(hg));
    const std::size_t size = SizeofResource(nullptr, rs);
    if (!data || size < sizeof(IrHeader)) { err = "invalid binaural IR resource"; return false; }
    const auto* h = reinterpret_cast<const IrHeader*>(data);
    if (std::memcmp(h->magic, "AUROHPIR", 8) || h->version != 1 ||
        h->rate != 48000 || rate != 48000) {
        err = "binaural mode currently requires 48000 Hz";
        return false;
    }
    if (room >= h->rooms) { err = "invalid room preset"; return false; }
    if ((bits != 16 && bits != 24 && bits != 32) || !channels ||
        channel_slots.size() < channels || !maximum_block_frames) {
        err = "unsupported PCM format";
        return false;
    }
    if (hrtf != 0u && hrtf != 2u) {
        err = "bundled binaural IR supports only HRTF preset 0=HPV2 or 2=GENERIC_2";
        return false;
    }
    const unsigned bank = hrtf == 0u ? 0u : 1u;
    if (bank >= h->banks) {
        err = "requested HRTF bank is absent from the binaural IR resource";
        return false;
    }

    auto impl = std::make_unique<Impl>();
    impl->bits = bits;
    impl->channels = channels;
    impl->bytes_per_sample = bits / 8;
    impl->maximum_block_frames = maximum_block_frames;
    impl->ir_frames = h->frames;
    impl->nfft = 1;
    while (impl->nfft < maximum_block_frames + h->frames - 1u)
        impl->nfft <<= 1u;
    impl->ir_left.resize(channels);
    impl->ir_right.resize(channels);
    impl->overlap_left.assign(h->frames ? h->frames - 1u : 0u, 0.0);
    impl->overlap_right.assign(h->frames ? h->frames - 1u : 0u, 0.0);

    const float* base = reinterpret_cast<const float*>(data + sizeof(IrHeader));
    const std::size_t ir_stride = static_cast<std::size_t>(h->frames) * 2u;
    constexpr std::size_t perf_index = 0;
    std::vector<C> ir(impl->nfft);
    for (unsigned ch = 0; ch < channels; ++ch) {
        int slot_index = -1;
        double remix_gain = 1.0;
        std::uint32_t slot = channel_slots[ch];
        if (slot == 7 || slot == 21) { slot = 4; remix_gain = 0.7071067811865476; }
        else if (slot == 8 || slot == 22) { slot = 5; remix_gain = 0.7071067811865476; }
        for (unsigned s = 0; s < h->slots; ++s)
            if (h->slot_ids[s] == slot) slot_index = static_cast<int>(s);
        if (slot_index < 0)
            continue;
        const std::size_t index = (((static_cast<std::size_t>(bank) * h->rooms + room)
            * h->perf + perf_index) * h->slots + static_cast<unsigned>(slot_index)) * ir_stride;
        for (unsigned ear = 0; ear < 2; ++ear) {
            std::fill(ir.begin(), ir.end(), C{});
            for (unsigned frame = 0; frame < h->frames; ++frame) {
                const unsigned block = frame / 32u;
                const unsigned position = frame % 32u;
                double value = base[index + block * 64u + ear * 32u + position] * remix_gain;
                if (frame + 1024u > h->frames)
                    value *= static_cast<double>(h->frames - frame) / 1024.0;
                ir[frame] = value;
            }
            fft(ir, false);
            (ear ? impl->ir_right[ch] : impl->ir_left[ch]) = ir;
        }
    }
    impl_ = std::move(impl);
    return true;
}

bool BinauralStreamRenderer::process(
    const std::vector<std::uint8_t>& pcm,
    std::vector<std::uint8_t>& out,
    std::string& err) {
    if (!impl_) { err = "binaural renderer is not initialized"; return false; }
    const std::size_t frame_bytes = static_cast<std::size_t>(impl_->channels) * impl_->bytes_per_sample;
    if (!frame_bytes || pcm.size() % frame_bytes) { err = "unsupported PCM format"; return false; }
    const std::size_t frames = pcm.size() / frame_bytes;
    if (frames > impl_->maximum_block_frames) { err = "binaural block is too large"; return false; }

    std::vector<C> sum_left(impl_->nfft), sum_right(impl_->nfft), input(impl_->nfft);
    for (unsigned ch = 0; ch < impl_->channels; ++ch) {
        if (impl_->ir_left[ch].empty())
            continue;
        std::fill(input.begin(), input.end(), C{});
        for (std::size_t frame = 0; frame < frames; ++frame)
            input[frame] = read_sample(&pcm[(frame * impl_->channels + ch) * impl_->bytes_per_sample], impl_->bits);
        fft(input, false);
        for (std::size_t k = 0; k < impl_->nfft; ++k) {
            sum_left[k] += input[k] * impl_->ir_left[ch][k];
            sum_right[k] += input[k] * impl_->ir_right[ch][k];
        }
    }
    fft(sum_left, true);
    fft(sum_right, true);

    out.clear();
    out.reserve(frames * 2u * impl_->bytes_per_sample);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const double old_left = frame < impl_->overlap_left.size() ? impl_->overlap_left[frame] : 0.0;
        const double old_right = frame < impl_->overlap_right.size() ? impl_->overlap_right[frame] : 0.0;
        write_sample(out, sum_left[frame].real() + old_left, impl_->bits);
        write_sample(out, sum_right[frame].real() + old_right, impl_->bits);
    }
    std::vector<double> next_left(impl_->overlap_left.size(), 0.0);
    std::vector<double> next_right(impl_->overlap_right.size(), 0.0);
    for (std::size_t k = 0; k < next_left.size(); ++k) {
        const std::size_t position = frames + k;
        if (position < impl_->overlap_left.size()) {
            next_left[k] += impl_->overlap_left[position];
            next_right[k] += impl_->overlap_right[position];
        }
        if (position < impl_->nfft) {
            next_left[k] += sum_left[position].real();
            next_right[k] += sum_right[position].real();
        }
    }
    impl_->overlap_left = std::move(next_left);
    impl_->overlap_right = std::move(next_right);
    return true;
}

bool render_binaural_from_embedded_ir(
    const std::vector<std::uint8_t>& pcm,
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::vector<std::uint8_t>& out,
    std::string& err,
    const ProgressFn& progress) {
    const unsigned bytes_per_sample = bits / 8u;
    if (!bytes_per_sample || !channels || pcm.size() % (channels * bytes_per_sample)) {
        err = "unsupported PCM format";
        return false;
    }
    const std::size_t frames = pcm.size() / (channels * bytes_per_sample);
    constexpr std::size_t kBlockFrames = 4096u;
    const std::size_t block_frames = std::min(kBlockFrames, frames ? frames : kBlockFrames);
    BinauralStreamRenderer renderer;
    if (!renderer.initialize(bits, rate, channels, channel_slots, room, hrtf, block_frames, err))
        return false;
    out.clear();
    out.reserve(frames * 2u * bytes_per_sample);
    const std::size_t in_frame_bytes = static_cast<std::size_t>(channels) * bytes_per_sample;
    std::vector<std::uint8_t> block_out;
    for (std::size_t frame = 0; frame < frames; frame += block_frames) {
        const std::size_t count = std::min(block_frames, frames - frame);
        const std::uint8_t* begin = pcm.data() + frame * in_frame_bytes;
        const std::vector<std::uint8_t> block_in(begin, begin + count * in_frame_bytes);
        if (!renderer.process(block_in, block_out, err))
            return false;
        out.insert(out.end(), block_out.begin(), block_out.end());
        if (progress)
            progress("encode binaural", progress_percent(frame + count, frames));
    }
    if (progress)
        progress("encode binaural", 100);
    return true;
}
}
