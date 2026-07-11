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

bool render_binaural_from_embedded_ir(const std::vector<std::uint8_t>& pcm,unsigned bits,unsigned rate,unsigned channels,
 const std::vector<std::uint32_t>& channel_slots,unsigned room,unsigned hrtf,std::vector<std::uint8_t>& out,std::string& err){
    HRSRC rs=FindResourceW(nullptr,MAKEINTRESOURCEW(102),MAKEINTRESOURCEW(10)); if(!rs){err="binaural IR resource not found";return false;}
    HGLOBAL hg=LoadResource(nullptr,rs); const auto* data=(const std::uint8_t*)LockResource(hg); std::size_t size=SizeofResource(nullptr,rs);
    if(!data||size<sizeof(IrHeader)){err="invalid binaural IR resource";return false;} const auto*h=(const IrHeader*)data;
    if(std::memcmp(h->magic,"AUROHPIR",8)||h->version!=1||h->rate!=48000||rate!=48000){err="binaural mode currently requires 48000 Hz";return false;}
    if(room>=h->rooms){err="invalid room preset";return false;} unsigned bank=hrtf==0?0:1; if(bank>=h->banks)bank=0;
    unsigned bps=bits/8; if((bits!=16&&bits!=24&&bits!=32)||!channels||pcm.size()%(channels*bps)){err="unsupported PCM format";return false;}
    std::size_t frames=pcm.size()/(channels*bps), nfft=1;while(nfft<frames+h->frames-1)nfft<<=1;
    std::vector<C> sumL(nfft),sumR(nfft),x(nfft),ir(nfft); const float* base=(const float*)(data+sizeof(IrHeader));
    const std::size_t ir_stride=std::size_t(h->frames)*2, perf_index=0;
    for(unsigned ch=0;ch<channels&&ch<channel_slots.size();++ch){int si=-1;double remix_gain=1.0;std::uint32_t slot=channel_slots[ch];
        // The native pipeline remixes layouts wider than its canonical 5.1.4
        // AHP input. Preserve its rear-to-surround -3 dB fold for 7.1/7.1.4.
        if(slot==7||slot==21){slot=4;remix_gain=0.7071067811865476;}else if(slot==8||slot==22){slot=5;remix_gain=0.7071067811865476;}
        for(unsigned s=0;s<h->slots;s++)if(h->slot_ids[s]==slot)si=(int)s;if(si<0)continue;
        std::fill(x.begin(),x.end(),C{});for(std::size_t f=0;f<frames;f++)x[f]=read_sample(&pcm[(f*channels+ch)*bps],bits)*remix_gain;fft(x,false);
        std::size_t idx=(((std::size_t(bank)*h->rooms+room)*h->perf+perf_index)*h->slots+si)*ir_stride;
        for(unsigned ear=0;ear<2;ear++){std::fill(ir.begin(),ir.end(),C{});for(unsigned f=0;f<h->frames;f++){unsigned block=f/32,pos=f%32;float v=base[idx+block*64+ear*32+pos];if(f+1024>h->frames)v*=double(h->frames-f)/1024.0;ir[f]=v;}fft(ir,false);auto&sum=ear?sumR:sumL;for(std::size_t k=0;k<nfft;k++)sum[k]+=x[k]*ir[k];}
    }
    fft(sumL,true);fft(sumR,true);out.clear();out.reserve(frames*2*bps);for(std::size_t f=0;f<frames;f++){write_sample(out,sumL[f].real(),bits);write_sample(out,sumR[f].real(),bits);}return true;
}
}
