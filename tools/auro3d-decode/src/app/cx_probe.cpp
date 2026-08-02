#include "cx_probe.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <utility>
namespace auro3d { namespace {
std::uint16_t be16(const std::uint8_t* p){ return std::uint16_t((p[0]<<8)|p[1]); }
std::uint32_t be32(const std::uint8_t* p){ return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3]; }
std::uint64_t be64(const std::uint8_t* p){ return (std::uint64_t(be32(p))<<32)|be32(p+4); }
bool is(const std::uint8_t* p,const char* s){ return p[0]==s[0]&&p[1]==s[1]&&p[2]==s[2]&&p[3]==s[3]; }
struct Box { std::uint64_t payload=0,end=0; const std::uint8_t* type=nullptr; };
bool next(const std::vector<std::uint8_t>& d,std::uint64_t& c,std::uint64_t lim,Box& b){
    if(c+8>lim||lim>d.size()) return false; std::uint64_t z=be32(d.data()+c),h=8; b.type=d.data()+c+4;
    if(z==1){ if(c+16>lim)return false; z=be64(d.data()+c+8);h=16; } else if(z==0) z=lim-c;
    if(z<h||z>lim-c)return false; b.payload=c+h;b.end=c+z;c=b.end;return true;
}
struct SampleToChunk { std::uint32_t first_chunk=0,samples_per_chunk=0; };
struct Track { bool audio=false,cx=false; std::uint16_t channels=0; std::uint32_t rate=0,count=0,duration=0; std::vector<std::uint32_t> sizes; std::vector<std::uint64_t> chunk_offsets,offsets; std::vector<SampleToChunk> sample_to_chunk; std::vector<std::uint8_t> acxd; };
void stsd(const std::vector<std::uint8_t>&d,const Box&b,Track&t){
    if(b.payload+8>b.end)return; std::uint64_t c=b.payload+8; Box e{}; if(!next(d,c,b.end,e)||!is(e.type,"a3ds"))return; t.cx=true;
    if(e.payload+28<=e.end){t.channels=be16(d.data()+e.payload+16);t.rate=be32(d.data()+e.payload+24)>>16;}
    c=e.payload+28; while(c+8<=e.end){Box x{};if(!next(d,c,e.end,x))break;if(is(x.type,"acxd"))t.acxd.assign(d.begin()+x.payload,d.begin()+x.end);}
}
void leaf(const std::vector<std::uint8_t>&d,const Box&b,Track&t){
    // Only mdia/hdlr=soun marks audio. Nested minf/hdlr (e.g. "url ") must not clear it.
    if(is(b.type,"hdlr")&&b.payload+12<=b.end&&is(d.data()+b.payload+8,"soun"))t.audio=true;
    else if(is(b.type,"stsd"))stsd(d,b,t);
    else if(is(b.type,"stts")&&b.payload+16<=b.end&&be32(d.data()+b.payload+4)){t.count=be32(d.data()+b.payload+8);t.duration=be32(d.data()+b.payload+12);}
    else if(is(b.type,"stsz")&&b.payload+12<=b.end){auto u=be32(d.data()+b.payload+4),n=be32(d.data()+b.payload+8);t.count=n;if(u)t.sizes.assign(n,u);else if(b.payload+12ull+4ull*n<=b.end)for(std::uint32_t i=0;i<n;++i)t.sizes.push_back(be32(d.data()+b.payload+12+4ull*i));}
    else if(is(b.type,"stsc")&&b.payload+8<=b.end){auto n=be32(d.data()+b.payload+4);if(b.payload+8ull+12ull*n<=b.end)for(std::uint32_t i=0;i<n;++i){auto*p=d.data()+b.payload+8+12ull*i;t.sample_to_chunk.push_back({be32(p),be32(p+4)});}}
    else if((is(b.type,"stco")||is(b.type,"co64"))&&b.payload+8<=b.end){bool w=is(b.type,"co64");auto n=be32(d.data()+b.payload+4);if(b.payload+8ull+(w?8ull:4ull)*n<=b.end)for(std::uint32_t i=0;i<n;++i){auto*p=d.data()+b.payload+8+(w?8ull:4ull)*i;t.chunk_offsets.push_back(w?be64(p):be32(p));}}
}
void walk(const std::vector<std::uint8_t>&d,std::uint64_t a,std::uint64_t z,Track&t){std::uint64_t c=a;while(c+8<=z){Box b{};if(!next(d,c,z,b))break;leaf(d,b,t);if(is(b.type,"mdia")||is(b.type,"minf")||is(b.type,"stbl"))walk(d,b.payload,b.end,t);}}
void build_sample_offsets(Track&t){
    t.offsets.clear();if(t.sizes.empty()||t.chunk_offsets.empty()||t.sample_to_chunk.empty())return;t.offsets.reserve(t.sizes.size());std::size_t sample=0,entry=0;
    for(std::size_t chunk=0;chunk<t.chunk_offsets.size()&&sample<t.sizes.size();++chunk){const std::uint32_t chunk_number=static_cast<std::uint32_t>(chunk+1);while(entry+1<t.sample_to_chunk.size()&&t.sample_to_chunk[entry+1].first_chunk<=chunk_number)++entry;std::uint64_t offset=t.chunk_offsets[chunk];for(std::uint32_t n=0;n<t.sample_to_chunk[entry].samples_per_chunk&&sample<t.sizes.size();++n,++sample){t.offsets.push_back(offset);offset+=t.sizes[sample];}}
}
bool xor_varint(const std::uint8_t*&p,const std::uint8_t*end,const std::uint8_t*key,unsigned&key_pos,std::uint32_t&value){
    value=0;
    for(unsigned n=0;n<5&&p<end;++n){
        const std::uint8_t byte=*p^key[key_pos&3];
        ++p;
        ++key_pos;
        value=(value<<7)|(byte&0x7f);
        if((byte&0x80)==0)
            return true;
    }
    return false;
}
struct Bits { const std::vector<std::uint8_t>&d;std::size_t p=0;bool get(unsigned n,std::uint32_t&v){if(p+n>d.size()*8||n>32)return false;v=0;for(unsigned i=0;i<n;++i)v|=((d[(p+i)/8]>>((p+i)&7))&1u)<<i;p+=n;return true;}bool unary(std::uint32_t&v){v=0;std::uint32_t x=0;while(get(1,x)){if(!x)return true;++v;}return false;}bool read_vlq(unsigned param,std::uint64_t&value){if(param<2||param>63)return false;const std::uint64_t continuation=std::uint64_t(1)<<(param-1u);const std::uint64_t payload_mask=continuation-1u;value=0;while(true){if(param>d.size()*8-p)return false;const unsigned low_width=std::min(param,32u);const unsigned high_width=param-low_width;std::uint32_t low=0,high=0;if(!get(low_width,low)||(high_width&&!get(high_width,high)))return false;const std::uint64_t field=std::uint64_t(low)|(std::uint64_t(high)<<32u);value=(value<<(param-1u))|(field&payload_mask);if((field&continuation)==0)return true;}} };
unsigned index_bits(std::uint32_t maximum){unsigned n=0;while(maximum){++n;maximum>>=1;}return n;}
bool block_size_id_is_alternating(std::uint32_t block_id){return block_id<=15u&&((33028u>>block_id)&1u)!=0u;}
std::uint32_t block_size_from_id(std::uint32_t block_id,std::uint32_t alt_index,std::uint32_t custom_size){
    if(block_id==31u)return custom_size;
    static const std::uint32_t kBlockSizes[33]={
        256u,400u,402u,480u,500u,512u,768u,800u,802u,960u,1000u,1001u,1024u,1536u,1600u,1602u,
        1920u,2000u,2002u,2048u,3072u,3840u,4096u,0u,0u,0u,0u,0u,0u,0u,0u,4095u,0u};
    if(block_id>=33u)return 0;
    if(block_id==2u)return alt_index?402u:400u;
    if(block_id==8u)return alt_index?802u:800u;
    if(block_id==15u)return alt_index?1602u:1600u;
    return kBlockSizes[block_id];
}
bool skip_bits(Bits&b,std::uint64_t count){std::uint32_t ignored=0;while(count){const unsigned chunk=static_cast<unsigned>(count>32?32:count);if(!b.get(chunk,ignored))return false;count-=chunk;}return true;}
std::int32_t sign_extend(std::uint32_t value,unsigned width){
    if(!width||width>=32)return static_cast<std::int32_t>(value);
    const std::uint32_t sign=1u<<(width-1u);
    return static_cast<std::int32_t>((value^sign)-sign);
}
bool read_integral_gain(Bits&b,unsigned width,AuroCxIntegralGainInfo&gain){
    std::uint32_t raw=0,selector=0;
    if(!b.get(2,selector))return false;
    gain.selector=selector;
    gain.value=0;
    if(selector<2u)return true;
    if(!b.get(width,raw))return false;
    gain.selector=0;
    gain.value=selector==2u?-1-static_cast<std::int32_t>(raw):1+static_cast<std::int32_t>(raw);
    return true;
}
bool skip_integral_gain_8(Bits&b){
    AuroCxIntegralGainInfo ignored{};
    return read_integral_gain(b,8,ignored);
}
bool skip_integral_gain_10(Bits&b){
    AuroCxIntegralGainInfo ignored{};
    return read_integral_gain(b,10,ignored);
}
bool skip_schema_language(Bits&b){return skip_bits(b,15);}
bool skip_schema_position(Bits&b,unsigned width){return skip_bits(b,3u*width);}
bool skip_schema_spread(Bits&b){return skip_bits(b,24);}
bool read_schema_position(Bits&b,unsigned width,AuroCxSchemaPositionInfo&position){
    std::uint32_t x=0,y=0,z=0;
    if(!b.get(width,x)||!b.get(width,y)||!b.get(width,z))return false;
    position.x=sign_extend(x,width);
    position.y=sign_extend(y,width);
    position.z=sign_extend(z,width);
    return true;
}
bool read_schema_spread(Bits&b,AuroCxSchemaSpreadInfo&spread){
    return b.get(8,spread.x)&&b.get(8,spread.y)&&b.get(8,spread.z);
}
bool read_subblock_gains(Bits&b,unsigned subblocks,std::vector<AuroCxSchemaGainSubblockInfo>&values){
    if(!subblocks)return false;
    values.assign(subblocks,{});
    values[0].changed=true;
    if(!read_integral_gain(b,10,values[0].absolute))return false;
    for(unsigned n=1;n<subblocks;++n){
        std::uint32_t changed=0,differential=0,raw=0;
        if(!b.get(1,changed))return false;
        values[n].changed=changed!=0;
        if(!changed)continue;
        if(!b.get(1,differential))return false;
        values[n].differential=differential!=0;
        if(differential){
            if(!b.get(6,raw))return false;
            values[n].delta=sign_extend(raw,6);
        }else if(!read_integral_gain(b,10,values[n].absolute))return false;
    }
    return true;
}
bool read_subblock_positions(Bits&b,unsigned subblocks,unsigned position_width,std::vector<AuroCxSchemaPositionSubblockInfo>&values){
    if(!subblocks)return false;
    values.assign(subblocks,{});
    values[0].changed=true;
    if(!read_schema_position(b,position_width,values[0].absolute))return false;
    for(unsigned n=1;n<subblocks;++n){
        std::uint32_t changed=0,differential=0;
        if(!b.get(1,changed))return false;
        values[n].changed=changed!=0;
        if(!changed)continue;
        if(!b.get(1,differential))return false;
        values[n].differential=differential!=0;
        if(differential){
            if(!read_schema_position(b,4,values[n].delta))return false;
        }else if(!read_schema_position(b,position_width,values[n].absolute))return false;
    }
    return true;
}
bool read_subblock_spreads(Bits&b,unsigned subblocks,std::vector<AuroCxSchemaSpreadSubblockInfo>&values){
    if(!subblocks)return false;
    values.assign(subblocks,{});
    values[0].changed=true;
    if(!read_schema_spread(b,values[0].absolute))return false;
    for(unsigned n=1;n<subblocks;++n){
        std::uint32_t changed=0;
        if(!b.get(1,changed))return false;
        values[n].changed=changed!=0;
        if(changed&&!read_schema_spread(b,values[n].absolute))return false;
    }
    return true;
}
bool skip_subblock_gains(Bits&b,unsigned subblocks){
    if(!subblocks||!skip_integral_gain_10(b))return false;
    for(unsigned n=1;n<subblocks;++n){
        std::uint32_t changed=0,differential=0;
        if(!b.get(1,changed))return false;
        if(changed){
            if(!b.get(1,differential))return false;
            if(differential){if(!skip_bits(b,6))return false;}
            else if(!skip_integral_gain_10(b))return false;
        }
    }
    return true;
}
bool skip_subblock_positions(Bits&b,unsigned subblocks,unsigned position_width){
    if(!subblocks||!skip_schema_position(b,position_width))return false;
    for(unsigned n=1;n<subblocks;++n){
        std::uint32_t changed=0,differential=0;
        if(!b.get(1,changed))return false;
        if(changed){
            if(!b.get(1,differential))return false;
            if(differential){if(!skip_bits(b,12))return false;}
            else if(!skip_schema_position(b,position_width))return false;
        }
    }
    return true;
}
bool skip_subblock_spreads(Bits&b,unsigned subblocks){
    if(!subblocks||!skip_schema_spread(b))return false;
    for(unsigned n=1;n<subblocks;++n){
        std::uint32_t changed=0;
        if(!b.get(1,changed))return false;
        if(changed&&!skip_schema_spread(b))return false;
    }
    return true;
}
template<class Payload>
bool read_object_group_optional(Bits&b,bool&present,bool&use_default,Payload&&payload){
    std::uint32_t present_bit=0;
    if(!b.get(1,present_bit))return false;
    present=present_bit!=0;
    if(!present)return true;
    std::uint32_t reuse_bit=0;
    if(!b.get(1,reuse_bit))return false;
    use_default=reuse_bit!=0;
    return use_default||payload();
}
bool read_mono_top_downmix_body(Bits&b,AuroCxMonoTopDownmixInfo&downmix){
    unsigned count=0u;
    if(downmix.kind==0u)count=4u;
    else if(downmix.kind==1u)count=3u;
    else if(downmix.kind==2u)count=1u;
    else return false;
    downmix.gains.assign(count,{});
    for(auto&gain:downmix.gains)
        if(!read_integral_gain(b,8,gain))return false;
    return true;
}
bool read_optional_mono_top_downmix(Bits&b,AuroCxMonoTopDownmixInfo&downmix){
    std::uint32_t present=0;
    if(!b.get(1,present))return false;
    downmix.present=present!=0;
    if(!present)return true;
    if(!b.get(2,downmix.kind))return false;
    return read_mono_top_downmix_body(b,downmix);
}
// StereoTopDownmix_t body (0x41AEF0): kind0→2 gains; kind2|3→1 gain.
bool read_stereo_top_downmix_body(Bits&b,AuroCxStereoTopDownmixInfo&downmix){
    unsigned count=0u;
    if(downmix.kind==0u)count=2u;
    else if(downmix.kind==2u||downmix.kind==3u)count=1u;
    else return false;
    downmix.gains.assign(count,{});
    for(auto&gain:downmix.gains)
        if(!read_integral_gain(b,8,gain))return false;
    return true;
}
bool read_optional_stereo_top_downmix(Bits&b,AuroCxStereoTopDownmixInfo&downmix){
    std::uint32_t present=0;
    if(!b.get(1,present))return false;
    downmix.present=present!=0;
    if(!present)return true;
    if(!b.get(2,downmix.kind))return false;
    return read_stereo_top_downmix_body(b,downmix);
}
bool read_optional_channel_gain(
    Bits&b,AuroCxChannelDownmixInfo&downmix,unsigned index){
    std::uint32_t present=0;
    if(!b.get(1,present))return false;
    if(downmix.gains.size()<=index)downmix.gains.resize(index+1u);
    if(!present)return true;
    downmix.gain_present_mask|=1u<<index;
    return read_integral_gain(b,8,downmix.gains[index]);
}
bool read_channel_downmix(Bits&b,std::uint32_t channel_id,AuroCxChannelDownmixInfo&downmix){
    constexpr std::uint32_t kMaskA=267382775u;
    constexpr std::uint32_t kMaskB=201556464u;
    constexpr std::uint32_t kMaskC=0xC0C01F7u;
    // ChannelDownmix decode 0x41A6B0: id==12 → MonoTop #1/#2; bits 28|29 →
    // StereoTop #3/#4 (mask 805306368). Do not treat 28/29 as MonoTop.
    if(channel_id==12u){
        downmix.mono_top.resize(2);
        return read_optional_mono_top_downmix(b,downmix.mono_top[0])&&
               read_optional_mono_top_downmix(b,downmix.mono_top[1]);
    }
    if(channel_id<32u&&((805306368u>>channel_id)&1u)){
        downmix.stereo_top.resize(2);
        return read_optional_stereo_top_downmix(b,downmix.stereo_top[0])&&
               read_optional_stereo_top_downmix(b,downmix.stereo_top[1]);
    }
    if(channel_id<32u&&((kMaskA>>channel_id)&1u)){
        if(!read_optional_channel_gain(b,downmix,0))return false;
        if(((1u<<channel_id)&kMaskC)!=0u){
            if(!read_optional_channel_gain(b,downmix,1))return false;
        }
        if(channel_id<=0x1Bu&&((kMaskB>>channel_id)&1u)){
            // Native type#7 at +152: present bit, then fixed IntegralGain count
            // from qword_1E4EA0[channel_id-4] (see decode ~0x41AE50).
            std::uint32_t present=0;
            if(!b.get(1,present))return false;
            downmix.intra_layer_present=present!=0;
            if(!present)return true;
            if(channel_id<4u||channel_id>27u)return false;
            static const std::uint8_t kIntraLayerCounts[24]={
                2,2,1,1,1,2,2,2,2,2,2,1,1,1,2,2,2,2,2,2,2,2,2,2};
            const std::uint8_t count=kIntraLayerCounts[channel_id-4u];
            downmix.intra_layer_gains.assign(count,{});
            for(auto&gain:downmix.intra_layer_gains)
                if(!read_integral_gain(b,8,gain))return false;
        }
        return true;
    }
    return true;
}
bool read_sasc_channel_bed(Bits&b,std::uint32_t beds,std::uint32_t objects,AuroCxSchemaPduInfo&pdu);
bool read_sasc_delta_header(
    Bits&b,std::uint32_t beds,std::uint32_t objects,
    std::uint32_t nr_audio_streams,
    const std::vector<AuroCxSchemaBedInfo>&bed_data,
    const std::vector<AuroCxSchemaObjectGroupInfo>&object_group_data,
    AuroCxSchemaPduInfo&pdu);
bool read_pdu_payload(Bits&b,AuroCxSchemaPduInfo&pdu,bool delta);
void finish_schema_parse(AuroCxProbeInfo&o,CxSchemaDecodeState*state,bool delta,unsigned audio_stream_bits,std::size_t pdu_vector_start,std::size_t consumed_bits,std::uint32_t gains_enabled=0,std::uint32_t explicit_metadata=0){
    if(delta){
        o.second_schema_consumed_bits=static_cast<std::uint32_t>(consumed_bits);
    }else{
        o.schema_consumed_bits=static_cast<std::uint32_t>(consumed_bits);
    }
    if(state&&!delta){
        state->valid=true;
        state->audio_stream_bits=audio_stream_bits;
        state->pdu_vector_bit_offset=pdu_vector_start;
        state->pdu_count=o.schema_pdu_count;
        state->pdu_types=o.schema_pdu_types;
        state->pdu_templates=o.schema_pdus;
        state->programs=o.schema_programs;
        state->beds=o.schema_beds;
        state->objects=o.schema_object_groups;
        state->switches=o.schema_switch_groups;
        state->block_size=o.schema_block_size;
        state->gains_enabled=gains_enabled;
        state->explicit_metadata=explicit_metadata;
        state->bed_channel_count=static_cast<std::uint32_t>(o.schema_bed_channels.size());
        state->last_awc_stream_count=0;
        for(const auto&template_pdu:o.schema_pdus){
            if(template_pdu.type==0&&template_pdu.audio_stream_count)
                state->last_awc_stream_count=template_pdu.audio_stream_count;
        }
    }
}
bool read_pdu_payload(Bits&b,AuroCxSchemaPduInfo&pdu,bool delta){
    std::uint32_t q=0;
    if(!b.unary(q)||q>55)return false;
    const unsigned suffix_bits=q+8u;
    std::uint64_t suffix=0;
    unsigned shift=0;
    while(shift<suffix_bits){
        const unsigned width=std::min(32u,suffix_bits-shift);
        std::uint32_t part=0;
        if(!b.get(width,part))return false;
        suffix|=std::uint64_t(part)<<shift;
        shift+=width;
    }
    const std::uint64_t base=((std::uint64_t(1)<<q)-1u)<<8u;
    pdu.payload_bits=base+suffix+1u;
    pdu.payload_bit_offset=static_cast<std::uint32_t>(b.p);
    if(delta)
        return skip_bits(b,pdu.payload_bits);
    pdu.payload_data.assign(static_cast<std::size_t>((pdu.payload_bits+7)/8),0);
    std::uint32_t tmp=0;
    for(std::uint64_t bit=0;bit<pdu.payload_bits;++bit){
        if(!b.get(1,tmp))return false;
        pdu.payload_data[static_cast<std::size_t>(bit/8)]|=static_cast<std::uint8_t>(tmp<<(bit&7));
    }
    if(pdu.type==0){
        const unsigned common_bits=pdu.header_flag0?9u:8u;
        const unsigned per_stream_bits=pdu.header_flag0?4u:12u;
        if(pdu.payload_bits==common_bits+std::uint64_t(per_stream_bits)*pdu.audio_stream_count){
            Bits payload{pdu.payload_data};
            if(payload.get(common_bits,pdu.awc_common_preamble)){
                pdu.awc_payload_config_decoded=true;
                for(std::uint32_t stream=0;stream<pdu.audio_stream_count;++stream){
                    std::uint32_t parameter=0;
                    if(!payload.get(per_stream_bits,parameter)){pdu.awc_payload_config_decoded=false;break;}
                    pdu.awc_stream_parameters.push_back(parameter);
                }
            }
        }
    }
    return true;
}
bool skip_optional_loudness_level(Bits&b){
    std::uint32_t flag=0;
    if(!b.get(1,flag))return false;
    if(flag&&!b.get(11,flag))return false;
    return true;
}
bool skip_schema_txt(Bits&b,unsigned length_bits){
    std::uint32_t ignored=0,has_length=0,length=0;
    if(!b.get(1,ignored)||!b.get(1,has_length))return false;
    if(has_length){
        if(length_bits&&!b.get(length_bits,length))return false;
    }else{
        if(length_bits>=32)return false;
        length=length_bits?((1u<<length_bits)-1u):0u;
    }
    return skip_bits(b,std::uint64_t(length)*8u);
}
bool skip_schema_names(Bits&b,unsigned length_bits){
    std::uint64_t count=0;
    if(!b.read_vlq(3,count))return false;
    ++count;
    for(std::uint64_t n=0;n<count;++n){
        if(!skip_bits(b,15)||!skip_schema_txt(b,length_bits))return false;
    }
    return true;
}
bool skip_loudness_data_set(Bits&b){
    for(unsigned n=0;n<9;++n)
        if(!skip_optional_loudness_level(b))return false;
    return true;
}
bool skip_loudness_t(Bits&b){
    std::uint32_t tmp=0;
    if(!b.get(3,tmp)||!b.get(2,tmp)||!b.get(2,tmp))return false;
    if(!b.get(1,tmp))return false;
    if(tmp&&!skip_loudness_data_set(b))return false;
    if(!b.get(1,tmp))return false;
    if(tmp&&!skip_loudness_data_set(b))return false;
    if(!b.get(1,tmp))return false;
    if(tmp&&!skip_loudness_data_set(b))return false;
    return true;
}
bool skip_program_optional_tail(Bits&b,std::uint32_t beds,bool delta,const CxSchemaDecodeState*state){
    std::uint32_t flag=0,tmp=0;
    if(!b.get(1,flag))return false;
    (void)delta;
    (void)state;
    if(flag&&!skip_loudness_t(b))return false;
    if(!b.get(1,flag))return false;
    if(flag){
        std::uint64_t count=0;
        if(!b.read_vlq(8,count)||!skip_bits(b,count*8))return false;
    }
    if(!b.get(1,flag))return false;
    if(flag){
        if(!b.get(1,tmp)||!b.get(2,tmp)||tmp==3)return false;
    }
    if(!b.get(1,flag))return false;
    if(flag){
        const unsigned bed_index_bits=index_bits(beds>0?beds-1:0);
        if(bed_index_bits&&!b.get(bed_index_bits,tmp))return false;
    }
    return true;
}
bool skip_schema_extension(Bits&b){
    const auto skip_base8_vlq=[&](){
        for(;;){
            std::uint32_t chunk=0;
            if(!b.get(4,chunk))return false;
            if(!(chunk&8u))return true;
        }
    };
    std::uint64_t payload_bytes=0;
    return skip_base8_vlq()&&skip_base8_vlq()&&
           b.read_vlq(8,payload_bytes)&&skip_bits(b,payload_bytes*8u);
}
bool parse_schema_object_group(
    Bits&b,unsigned name_length_bits,bool gains_enabled,bool explicit_metadata,
    unsigned audio_stream_bits,unsigned metadata_subblocks,unsigned position_width,
    AuroCxSchemaObjectGroupInfo&group){
    std::uint32_t flag=0,tmp=0;
    if(name_length_bits){
        if(!b.get(1,flag))return false;
        if(flag&&!skip_schema_names(b,name_length_bits))return false;
    }
    if(!b.get(1,flag))return false;
    if(flag&&!skip_schema_language(b))return false;
    if(explicit_metadata){
        if(!b.get(1,flag))return false;
        if(flag&&!skip_bits(b,10))return false;
    }
    bool group_gains=false,group_positions=false,group_spreads=false;
    if(gains_enabled&&!read_object_group_optional(
           b,group_gains,group.gains_use_default,
           [&](){return read_subblock_gains(b,metadata_subblocks,group.gains);} ))return false;
    group.gains_present=group_gains;
    if(explicit_metadata){
        if(!b.get(1,flag))return false;
        if(flag&&!skip_bits(b,30))return false;
    }
    if(!read_object_group_optional(
           b,group_positions,group.positions_use_default,
           [&](){return read_subblock_positions(b,metadata_subblocks,position_width,group.positions);} ))return false;
    group.positions_present=group_positions;
    if(!read_object_group_optional(
           b,group_spreads,group.spreads_use_default,
           [&](){return read_subblock_spreads(b,metadata_subblocks,group.spreads);} ))return false;
    group.spreads_present=group_spreads;
    if(!b.get(1,flag))return false;
    if(flag&&!b.get(1,tmp))return false;
    std::uint64_t object_count=0;
    if(!b.read_vlq(3,object_count)||object_count>b.d.size()*8u-b.p)return false;
    group.objects.clear();
    group.objects.reserve(static_cast<std::size_t>(object_count));
    for(std::uint64_t object=0;object<object_count;++object){
        AuroCxSchemaObjectInfo info{};
        if(!b.get(1,flag))return false;
        info.content_kind_present=flag!=0;
        if(flag){
            if(!b.read_vlq(8,info.content_kind)||
               !b.get(4,info.content_kind_classifier)||info.content_kind_classifier>8u)return false;
        }
        if(!b.get(audio_stream_bits,info.audio_stream_index)||!b.get(1,tmp))return false;
        info.flag0=tmp!=0;
        if(!b.get(1,tmp)||!read_integral_gain(b,10,info.gain))return false;
        info.flag1=tmp!=0;
        if(gains_enabled&&!group_gains){
            if(!b.get(1,flag))return false;
            info.gains_use_default=flag!=0;
            if(!flag&&!read_subblock_gains(b,metadata_subblocks,info.gains))return false;
        }
        if(!group_positions){
            if(!b.get(1,flag))return false;
            info.positions_use_default=flag!=0;
            if(!flag&&!read_subblock_positions(b,metadata_subblocks,position_width,info.positions))return false;
        }
        if(!group_spreads){
            if(!b.get(1,flag))return false;
            info.spreads_use_default=flag!=0;
            if(!flag&&!read_subblock_spreads(b,metadata_subblocks,info.spreads))return false;
        }
        if(!b.get(1,flag))return false;
        info.zone_exclusion_present=flag!=0;
        if(flag){
            info.zone_exclusion_gains.assign(16,{});
            for(unsigned zone=0;zone<16;++zone){
                if(!b.get(1,tmp))return false;
                if(tmp){
                    info.zone_exclusion_mask|=1u<<zone;
                    if(!read_integral_gain(b,8,info.zone_exclusion_gains[zone]))return false;
                }
            }
        }
        group.objects.push_back(std::move(info));
    }
    return true;
}
bool parse_schema_reference(
    Bits&b,bool gains_enabled,bool explicit_metadata,unsigned index_width,
    AuroCxSchemaReferenceInfo&reference){
    std::uint32_t value=0;
    if(explicit_metadata){
        if(!b.get(1,value))return false;
        reference.explicit_metadata_flag=value!=0;
    }
    if(gains_enabled&&!read_integral_gain(b,10,reference.gain))return false;
    return b.get(index_width,reference.index);
}
bool parse_schema_switch_group(
    Bits&b,unsigned name_length_bits,bool gains_enabled,bool explicit_metadata,
    std::uint32_t beds,std::uint32_t object_groups,AuroCxSchemaSwitchGroupInfo&group){
    std::uint32_t flag=0;
    if(name_length_bits){
        if(!b.get(1,flag))return false;
        if(flag&&!skip_schema_names(b,name_length_bits))return false;
    }
    std::uint64_t element_count=0;
    if(!b.read_vlq(3,element_count)||element_count>b.d.size()*8u-b.p)return false;
    group.elements.clear();
    group.elements.reserve(static_cast<std::size_t>(element_count));
    const unsigned object_count_width=index_bits(object_groups);
    const unsigned object_index_width=index_bits(object_groups?object_groups-1u:0u);
    const unsigned bed_count_width=index_bits(beds);
    const unsigned bed_index_width=index_bits(beds?beds-1u:0u);
    for(std::uint64_t element=0;element<element_count;++element){
        if(name_length_bits){
            if(!b.get(1,flag))return false;
            if(flag&&!skip_schema_names(b,name_length_bits))return false;
        }
        if(!b.get(1,flag))return false;
        if(flag&&!skip_schema_language(b))return false;
        AuroCxSchemaSwitchGroupElementInfo info{};
        std::uint32_t count=0;
        if(!b.get(object_count_width,count)||count>object_groups)return false;
        info.object_groups.reserve(count);
        info.object_group_references.reserve(count);
        for(std::uint32_t n=0;n<count;++n){
            AuroCxSchemaReferenceInfo reference{};
            if(!parse_schema_reference(b,gains_enabled,explicit_metadata,object_index_width,reference)||reference.index>=object_groups)return false;
            info.object_groups.push_back(reference.index);
            info.object_group_references.push_back(reference);
        }
        if(!b.get(bed_count_width,count)||count>beds)return false;
        info.beds.reserve(count);
        info.bed_references.reserve(count);
        for(std::uint32_t n=0;n<count;++n){
            AuroCxSchemaReferenceInfo reference{};
            if(!parse_schema_reference(b,gains_enabled,explicit_metadata,bed_index_width,reference)||reference.index>=beds)return false;
            info.beds.push_back(reference.index);
            info.bed_references.push_back(reference);
        }
        group.elements.push_back(std::move(info));
    }
    return true;
}
bool read_sasc_channel_bed(Bits&b,std::uint32_t beds,std::uint32_t objects,AuroCxSchemaPduInfo&pdu){
    std::uint32_t tmp=0;
    const unsigned bed_bits=index_bits(beds>0?beds-1:0);
    if(bed_bits&&!b.get(bed_bits,pdu.sasc_bed_index))return false;
    if(pdu.sasc_bed_index>=beds)return false;
    if(!b.get(2,pdu.sasc_channel_bed_layer))return false;
    if(!b.get(1,tmp))return false;
    pdu.sasc_config_flag=tmp!=0;
    const unsigned object_count_bits=index_bits(objects);
    std::uint32_t linked_count=0;
    if(object_count_bits&&!b.get(object_count_bits,linked_count))return false;
    const unsigned object_index_bits=index_bits(objects>0?objects-1:0);
    pdu.sasc_linked_object_groups.clear();
    pdu.sasc_linked_object_groups.reserve(linked_count);
    for(std::uint32_t i=0;i<linked_count;++i){
        std::uint32_t idx=0;
        if(object_index_bits&&!b.get(object_index_bits,idx))return false;
        if(idx>=objects)return false;
        pdu.sasc_linked_object_groups.push_back(idx);
    }
    pdu.sasc_channel_bed_decoded=true;
    return true;
}
bool read_sasc_flag(Bits&b,bool&flag){
    std::uint32_t tmp=0;
    if(!b.get(1,tmp))return false;
    flag=tmp!=0;
    return true;
}
bool build_sasc_canonical_streams(
    AuroCxSchemaPduInfo&pdu,
    const std::vector<AuroCxSchemaBedInfo>&beds,
    const std::vector<AuroCxSchemaObjectGroupInfo>&object_groups){
    pdu.sasc_audio_stream_indices.clear();
    if(pdu.header_value==0){
        if(pdu.sasc_object_group_index>=object_groups.size())return false;
        for(const auto&object:object_groups[pdu.sasc_object_group_index].objects)
            pdu.sasc_audio_stream_indices.push_back(object.audio_stream_index);
        return true;
    }
    if(pdu.sasc_bed_index>=beds.size())return false;
    if(pdu.header_value==1&&beds[pdu.sasc_bed_index].ambisonics)return false;
    if(pdu.header_value==2&&!beds[pdu.sasc_bed_index].ambisonics)return false;
    for(const auto&channel:beds[pdu.sasc_bed_index].channels)
        if(pdu.header_value==2||channel.layer_index==pdu.sasc_channel_bed_layer)
            pdu.sasc_audio_stream_indices.push_back(channel.audio_stream_index);
    for(const auto index:pdu.sasc_linked_object_groups){
        if(index>=object_groups.size())return false;
        for(const auto&object:object_groups[index].objects)
            pdu.sasc_audio_stream_indices.push_back(object.audio_stream_index);
    }
    return true;
}
bool read_sasc_scg_bitset(
    Bits&b,std::uint32_t nr_channels,std::uint32_t nr_audio_streams,
    std::vector<std::vector<bool>>&channel_sets,
    std::vector<std::vector<std::uint32_t>>&stream_indices){
    channel_sets.clear();
    stream_indices.clear();
    bool deep_tree=false;
    if(!read_sasc_flag(b,deep_tree))return false;
    const std::uint32_t level_count=deep_tree?2u:1u;
    channel_sets.reserve(level_count);
    for(std::uint32_t level=0;level<level_count;++level){
        bool all_enabled=false;
        if(!read_sasc_flag(b,all_enabled))return false;
        std::vector<bool> enabled(nr_channels,all_enabled);
        if(!all_enabled){
            for(std::uint32_t ch=0;ch<nr_channels;++ch){
                std::uint32_t value=0;
                if(!b.get(1,value))return false;
                enabled[ch]=value!=0;
            }
        }
        channel_sets.push_back(std::move(enabled));
    }
    std::uint64_t enabled_count=0;
    for(const auto&level:channel_sets)
        for(const bool enabled:level)
            enabled_count+=enabled?1u:0u;
    if(enabled_count>nr_audio_streams)return false;
    std::uint32_t next_stream=nr_audio_streams-static_cast<std::uint32_t>(enabled_count);
    stream_indices.reserve(channel_sets.size());
    for(const auto&level:channel_sets){
        std::vector<std::uint32_t> indices(nr_channels,UINT32_MAX);
        for(std::uint32_t channel=0;channel<nr_channels;++channel)
            if(level[channel])indices[channel]=next_stream++;
        stream_indices.push_back(std::move(indices));
    }
    if(next_stream!=nr_audio_streams)return false;
    return true;
}
bool read_sasc_delta_header(
    Bits&b,std::uint32_t beds,std::uint32_t objects,
    std::uint32_t nr_audio_streams,
    const std::vector<AuroCxSchemaBedInfo>&bed_data,
    const std::vector<AuroCxSchemaObjectGroupInfo>&object_group_data,
    AuroCxSchemaPduInfo&pdu){
    std::uint32_t channel_bed_layer=0;
    if(!b.get(2,pdu.header_value)||pdu.header_value==3)return false;
    if(pdu.header_value==1){
        if(!read_sasc_channel_bed(b,beds,objects,pdu))return false;
        channel_bed_layer=pdu.sasc_channel_bed_layer;
    }else if(pdu.header_value==0){
        const unsigned width=index_bits(objects>0?objects-1:0);
        if(width&&!b.get(width,pdu.sasc_object_group_index))return false;
        if(pdu.sasc_object_group_index>=objects)return false;
    }else if(pdu.header_value==2){
        const unsigned width=index_bits(beds>0?beds-1:0);
        if(width&&!b.get(width,pdu.sasc_bed_index))return false;
        if(pdu.sasc_bed_index>=beds)return false;
    }else return false;
    bool resampled=false;
    if(!read_sasc_flag(b,resampled))return false;
    pdu.sasc_resample_factor=1;
    if(resampled){
        bool second=false;
        if(!read_sasc_flag(b,second))return false;
        pdu.sasc_resample_factor=second?4u:2u;
    }
    if(pdu.header_value==1){
        std::uint32_t mode_flag=0;
        if(!b.get(1,mode_flag))return false;
        pdu.sasc_mode_flag=mode_flag!=0;
    }
    if(!build_sasc_canonical_streams(pdu,bed_data,object_group_data))return false;
    const std::uint32_t nr_channels=static_cast<std::uint32_t>(pdu.sasc_audio_stream_indices.size());
    bool has_channel_set=false;
    if(!read_sasc_flag(b,has_channel_set))return false;
    if(!has_channel_set){
        pdu.sasc_channel_sets.clear();
        pdu.sasc_channel_set_stream_indices.clear();
        return true;
    }
    return read_sasc_scg_bitset(
        b,nr_channels,nr_audio_streams,
        pdu.sasc_channel_sets,pdu.sasc_channel_set_stream_indices);
}
bool parse_schema_header(const std::vector<std::uint8_t>&d,AuroCxProbeInfo&o,CxSchemaDecodeState*state=nullptr,bool delta=false){
    if(delta)o.second_schema_blob_bits=static_cast<std::uint32_t>(d.size()*8);
    else o.schema_blob_bits=static_cast<std::uint32_t>(d.size()*8);
    Bits b{d};std::uint32_t v=0,profile=0,sample_rate_id=0,flag=0,block_id=0,tmp=0,custom_block_size=0;
    if(!b.unary(v)||!b.unary(profile)||!b.get(3,sample_rate_id))return false;if(sample_rate_id==7&&!b.get(20,tmp))return false;
    if(!b.get(1,flag)||!b.get(5,block_id))return false;
    if(block_id==31u){if(!b.get(12,tmp))return false;custom_block_size=tmp+1u;}else if(block_id>=24u)return false;
    std::uint32_t block_alt_index=0;
    if(block_size_id_is_alternating(block_id)){if(!b.get(1,block_alt_index))return false;}
    o.schema_block_size=block_size_from_id(block_id,block_alt_index,custom_block_size);
    if(!b.get(3,tmp)||!b.get(1,flag))return false;if(flag&&!b.get(12,tmp))return false;
    std::uint32_t audio_stream_bits=0,bed_names_enabled=0;
    if(!b.unary(audio_stream_bits)||!b.unary(bed_names_enabled)||!b.get(1,flag))return false;if(flag&&!b.get(16,tmp))return false;
    if(state&&(!delta||!state->valid))state->audio_stream_bits=audio_stream_bits;
    std::uint32_t common=0;if(!b.get(1,common))return false;o.schema_common_config=common!=0;
    std::uint32_t gains_enabled=0,explicit_metadata=0,programs=1,beds=0,objects=0,switches=0;
    bool object_position_16bit=false;
    bool default_object_position_present=false;
    AuroCxSchemaPositionInfo default_object_position{};
    if(common){
        if(!delta||!state||!state->valid)return false;
        gains_enabled=state->gains_enabled;explicit_metadata=state->explicit_metadata;
        programs=state->programs;beds=state->beds;objects=state->objects;switches=state->switches;
        object_position_16bit=state->object_position_16bit;
        default_object_position_present=state->default_object_position_present;
        default_object_position=state->default_object_position;
    }else{
        if(!b.get(1,gains_enabled)||!b.get(1,explicit_metadata))return false;
        if(explicit_metadata&&!b.unary(programs))return false;if(explicit_metadata)++programs;
        if(!b.unary(beds)||!b.unary(objects))return false;if(explicit_metadata&&!b.unary(switches))return false;
    }
    o.schema_header_decoded=true;o.schema_codec_version=v;o.schema_codec_profile=profile;o.schema_programs=programs;o.schema_beds=beds;o.schema_object_groups=objects;o.schema_switch_groups=switches;
    const unsigned program_index_bits=index_bits(programs-1);
    if(!b.get(program_index_bits,o.schema_primary_program_index)||
       !b.get(program_index_bits,o.schema_default_program_index)||
       o.schema_primary_program_index>=programs||
       o.schema_default_program_index>=programs)return true;
    if(!common&&objects){
        if(!b.get(1,flag))return true;
        object_position_16bit=flag!=0;
        default_object_position={0,object_position_16bit?32767:127,0};
        if(!b.get(1,flag))return true;
        default_object_position_present=flag!=0;
        if(flag&&!read_schema_position(
               b,object_position_16bit?16u:8u,default_object_position))return true;
    }
    if(state&&!delta){
        state->object_position_16bit=object_position_16bit;
        state->default_object_position_present=default_object_position_present;
        state->default_object_position=default_object_position;
    }
    o.schema_gains_enabled=gains_enabled!=0;
    o.schema_explicit_metadata=explicit_metadata!=0;
    o.schema_object_position_16bit=object_position_16bit;
    o.schema_default_object_position_present=default_object_position_present;
    o.schema_default_object_position=default_object_position;
    o.schema_metadata_subblocks=1u<<sample_rate_id;
    std::uint32_t extensions=0;if(!b.unary(extensions))return true;
    o.schema_config_header_bits=static_cast<std::uint32_t>(b.p);
    if(delta)o.second_schema_config_header_bits=o.schema_config_header_bits;
    o.schema_program_data.clear();
    o.schema_program_data.reserve(programs);
    for(std::uint32_t program=0;program<programs;++program){
        AuroCxSchemaProgramInfo program_info{};
        if(!b.get(1,flag))return true;if(flag&&!b.unary(tmp))return true;
        if(bed_names_enabled){if(!b.get(1,flag))return true;if(flag&&!skip_schema_names(b,bed_names_enabled))return true;}
        if(!b.get(1,flag))return true;if(flag&&!b.get(4,tmp))return true;if(!b.get(1,flag))return true;
        std::uint32_t refs=0;if(!b.get(index_bits(beds),refs)||refs>beds)return true;if(program==0)o.schema_program0_bed_references=refs;
        program_info.beds.reserve(refs);program_info.bed_references.reserve(refs);
        for(std::uint32_t n=0;n<refs;++n){AuroCxSchemaReferenceInfo reference{};if(!parse_schema_reference(b,gains_enabled!=0,explicit_metadata!=0,index_bits(beds>0?beds-1:0),reference)||reference.index>=beds)return true;program_info.beds.push_back(reference.index);program_info.bed_references.push_back(reference);}
        if(!b.get(index_bits(objects),refs)||refs>objects)return true;
        program_info.object_groups.reserve(refs);program_info.object_group_references.reserve(refs);
        for(std::uint32_t n=0;n<refs;++n){AuroCxSchemaReferenceInfo reference{};if(!parse_schema_reference(b,gains_enabled!=0,explicit_metadata!=0,index_bits(objects>0?objects-1:0),reference)||reference.index>=objects)return true;program_info.object_groups.push_back(reference.index);program_info.object_group_references.push_back(reference);}
        if(!b.get(index_bits(switches),refs)||refs>switches)return true;
        program_info.switch_groups.reserve(refs);
        for(std::uint32_t n=0;n<refs;++n){
            if(!b.get(index_bits(switches>0?switches-1:0),tmp)||tmp>=switches)return true;
            program_info.switch_groups.push_back(tmp);
        }
        if(!skip_program_optional_tail(b,beds,delta,state))return true;
        o.schema_program_data.push_back(std::move(program_info));
    }
    o.schema_programs_end_bit=static_cast<std::uint32_t>(b.p);
    if(delta)o.second_schema_programs_end_bit=o.schema_programs_end_bit;
    o.schema_bed_data.clear();
    o.schema_bed_data.reserve(beds);
    for(std::uint32_t bed=0;bed<beds;++bed){
        if(bed_names_enabled){if(!b.get(1,flag))return true;if(flag&&!skip_schema_names(b,bed_names_enabled))return true;}
        if(explicit_metadata){if(!b.get(1,flag))return true;if(flag){if(!b.get(1,tmp)||!b.get(5,tmp)||!b.get(4,tmp))return true;}}
        if(!b.get(1,flag))return true;if(flag&&!b.get(1,tmp))return true;
        std::uint32_t ambisonics=0;if(!b.get(1,ambisonics))return true;
        std::vector<AuroCxSchemaChannelInfo> parsed;
        if(ambisonics){
            static const std::uint32_t channel_counts[]={4u,9u,16u};
            std::uint32_t order=0,normalization=0;
            if(!b.get(2,order)||order>=3u||!b.get(2,normalization)||normalization>1u)return true;
            const std::uint32_t channels=channel_counts[order];
            std::uint32_t component_mask=0;
            parsed.reserve(channels);
            for(std::uint32_t n=0;n<channels;++n){
                std::uint32_t component=0,stream=0;
                if(!b.get(4,component)||!b.get(audio_stream_bits,stream)||component>=channels)return true;
                component_mask|=1u<<component;
                parsed.push_back({component,stream,0});
                if(bed==0)o.schema_channel_end_bits.push_back(static_cast<std::uint32_t>(b.p));
            }
            if(component_mask!=((1u<<channels)-1u))return true;
        }else{
            std::uint32_t layers=0;if(!b.unary(layers))return true;++layers;if(layers>4)return true;
            if(!b.get(1,flag))return true;if(flag&&layers!=1&&!b.get(2,tmp))return true;
            std::uint32_t channels=0;if(!b.get(6,channels))return true;
            parsed.reserve(channels);
            for(std::uint32_t n=0;n<channels;++n){
                std::uint32_t groups=0,low=0,stream=0,layer=0,flag0=0,flag1=0;
                if(!b.unary(groups)||!b.get(2,low)||!b.get(audio_stream_bits,stream))return true;
                const std::uint64_t channel_id=4ull*groups+low;
                if(channel_id>30u)return true;
                if(layers>1&&(!b.get(index_bits(layers-1),layer)||layer>=layers))return true;
                AuroCxSchemaChannelInfo channel{};
                channel.id=static_cast<std::uint32_t>(channel_id);
                channel.audio_stream_index=stream;
                channel.layer_index=layer;
                if(gains_enabled&&!read_integral_gain(b,10,channel.gain))return true;
                if(!b.get(1,flag0)||!b.get(1,flag1))return true;
                channel.flag0=flag0!=0;
                channel.downmix_present=flag1!=0;
                if(flag1){
                    channel.downmix_bit_offset=static_cast<std::uint32_t>(b.p);
                    if(!read_channel_downmix(b,channel.id,channel.downmix))return true;
                    channel.downmix_bits=static_cast<std::uint32_t>(b.p)-channel.downmix_bit_offset;
                }
                parsed.push_back(std::move(channel));
                if(bed==0)o.schema_channel_end_bits.push_back(static_cast<std::uint32_t>(b.p));
            }
        }
        if(bed==0){o.schema_bed_channels=parsed;o.schema_bed_channels_decoded=true;}
        o.schema_bed_data.push_back({ambisonics!=0,std::move(parsed)});
    }
    o.schema_object_group_data.clear();
    o.schema_object_group_data.reserve(objects);
    const unsigned metadata_subblocks=1u<<sample_rate_id;
    const unsigned position_width=object_position_16bit?16u:8u;
    for(std::uint32_t object_group=0;object_group<objects;++object_group){
        AuroCxSchemaObjectGroupInfo parsed{};
        if(!parse_schema_object_group(
               b,bed_names_enabled,gains_enabled!=0,explicit_metadata!=0,
               audio_stream_bits,metadata_subblocks,position_width,parsed))return true;
        o.schema_object_group_data.push_back(std::move(parsed));
    }
    o.schema_switch_group_data.clear();
    o.schema_switch_group_data.reserve(switches);
    for(std::uint32_t switch_group=0;switch_group<switches;++switch_group){
        AuroCxSchemaSwitchGroupInfo parsed{};
        if(!parse_schema_switch_group(
               b,bed_names_enabled,gains_enabled!=0,explicit_metadata!=0,
               beds,objects,parsed))return true;
        o.schema_switch_group_data.push_back(std::move(parsed));
    }
    for(std::uint32_t extension=0;extension<extensions;++extension)
        if(!skip_schema_extension(b))return true;
    o.schema_audio_bit_offset=static_cast<std::uint32_t>(b.p);
    if(delta)o.second_schema_audio_bit_offset=o.schema_audio_bit_offset;
    const std::size_t pdu_vector_start=b.p;
    std::uint32_t quotient=0,remainder=0;
    if(!b.unary(quotient)||!b.get(audio_stream_bits,remainder)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
    o.schema_pdu_count=(quotient<<audio_stream_bits)|remainder;
    std::uint32_t next_audio_stream=0;
    for(std::uint32_t n=0;n<o.schema_pdu_count;++n){
        AuroCxSchemaPduInfo pdu{};
        std::uint32_t type=0;
        if(!b.get(3,type)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
        pdu.type=type;
        o.schema_pdu_types.push_back(pdu.type);
        bool supported=true;
        if(pdu.type==0){
            if(!b.get(audio_stream_bits,tmp)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;
            if(!b.get(1,pdu.header_flag0)||!b.get(1,pdu.header_flag1)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            if(pdu.header_flag1&&!b.get(1,pdu.header_value)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
        }else if(pdu.type==1){
            if(!b.get(audio_stream_bits,tmp)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;
        }else if(pdu.type==2){
            if(!b.get(audio_stream_bits,tmp)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;
            if(!b.get(1,pdu.header_flag0)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            if(pdu.header_flag0&&!b.get(3,pdu.header_value)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            if(pdu.header_flag0&&pdu.header_value>4u){
                o.schema_pdus.push_back(pdu);
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
        }else if(pdu.type==3){
            const unsigned bed_index_bits=index_bits(beds>0?beds-1:0);
            if((bed_index_bits&&!b.get(bed_index_bits,pdu.transform_bed_index))||
               !b.get(2,pdu.transform_source_layer)||!b.get(2,pdu.transform_target_layer)){
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
            pdu.header_value=pdu.transform_source_layer;
        }else if(pdu.type==4){
            if(!b.get(audio_stream_bits,tmp)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;
            const unsigned bed_index_bits=index_bits(beds>0?beds-1:0);
            if((bed_index_bits&&!b.get(bed_index_bits,pdu.transform_bed_index))||
               !b.get(2,pdu.transform_target_layer)){
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
            pdu.header_value=pdu.transform_target_layer;
        }else if(pdu.type==5){
            const unsigned bed_index_bits=index_bits(beds>0?beds-1:0);
            if((bed_index_bits&&!b.get(bed_index_bits,pdu.transform_bed_index))||
               !b.get(2,pdu.transform_source_layer)||!b.get(2,pdu.transform_target_layer)||
               !b.get(1,pdu.header_flag1)){
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
            if(pdu.header_flag1&&!b.get(1,pdu.header_flag0)){
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
            pdu.header_value=pdu.transform_source_layer;
            pdu.transform_resample_factor=pdu.header_flag1?(pdu.header_flag0?4u:2u):1u;
        }else if(pdu.type==6){
            if(!read_sasc_delta_header(
                   b,beds,objects,next_audio_stream,
                   o.schema_bed_data,o.schema_object_group_data,pdu)){
                o.schema_pdus.push_back(pdu);
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
        }else if(pdu.type==7){
            if(!b.get(audio_stream_bits,tmp)){finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
            pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;
            const auto read_base4_vlq=[&](std::uint32_t&value){
                value=0;
                for(;;){
                    std::uint32_t chunk=0;
                    if(!b.get(3,chunk))return false;
                    if(value>(UINT32_MAX-(chunk&3u))/4u)return false;
                    value=4u*value+(chunk&3u);
                    if(!(chunk&4u))return true;
                }
            };
            std::uint32_t extension_id=0,index_width=0;
            if(!read_base4_vlq(extension_id)||!read_base4_vlq(index_width)){
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
            pdu.header_value=extension_id;
            pdu.header_flag0=index_width;
            if(index_width>32u){
                if(!skip_bits(b,std::uint64_t(index_width)*pdu.audio_stream_count)){
                    o.schema_pdus.push_back(pdu);
                    finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
                }
                o.schema_pdus.push_back(pdu);
                continue;
            }
            std::int64_t prior=-1;
            for(std::uint32_t stream=0;stream<pdu.audio_stream_count;++stream){
                std::uint32_t raw=0;
                if(index_width&&!b.get(index_width,raw)){
                    o.schema_pdus.push_back(pdu);
                    finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
                }
                std::int64_t value=0;
                if(index_width){
                    const std::uint64_t sign=std::uint64_t(1)<<(index_width-1u);
                    value=(std::uint64_t(raw)&sign)
                        ? static_cast<std::int64_t>(raw)-static_cast<std::int64_t>(std::uint64_t(1)<<index_width)
                        : static_cast<std::int64_t>(raw);
                }
                if(stream)value+=prior+1;
                prior=value;
            }
            if(b.p>std::uint64_t(d.size())*8u){
                o.schema_pdus.push_back(pdu);
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
        }else supported=false;
        if(pdu.type<=5){
            if(!supported||!read_pdu_payload(b,pdu,delta)){
                o.schema_pdus.push_back(pdu);
                finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;
            }
        }
        o.schema_pdus.push_back(pdu);
        if(pdu.type>5&&!supported){if(n+1==o.schema_pdu_count)o.schema_pdu_types_complete=true;finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p);return true;}
    }
    o.schema_pdu_types_complete=true;
    finish_schema_parse(o,state,delta,audio_stream_bits,pdu_vector_start,b.p,gains_enabled,explicit_metadata);
    return true;
}
void parse_segments(const std::uint8_t*p,const std::uint8_t*end,AuroCxProbeInfo&o,CxSchemaDecodeState*state=nullptr,bool delta=false){
    while(end-p>=8&&p[0]==0xA3&&p[1]==0xDC&&p[2]==0x0D&&p[3]==0xED){const std::uint8_t*key=p+4;const std::uint8_t*q=p+8;unsigned key_pos=0;std::uint32_t id=0,len=0;if(!xor_varint(q,end,key,key_pos,id)||!xor_varint(q,end,key,key_pos,len)||std::uint64_t(end-q)<len)break;o.first_access_unit_segments.push_back({id,len});if(id==1){std::vector<std::uint8_t>decoded(len);for(std::uint32_t i=0;i<len;++i)decoded[i]=q[i]^key[(key_pos+i)&3];parse_schema_header(decoded,o,state,delta);}p=q+len;}
}
void fill_schema_result(const AuroCxProbeInfo&o,CxSchemaParseResult&out,bool delta){
    out.header_decoded=o.schema_header_decoded;
    out.common_config=o.schema_common_config;
    out.gains_enabled=o.schema_gains_enabled;
    out.explicit_metadata=o.schema_explicit_metadata;
    out.object_position_16bit=o.schema_object_position_16bit;
    out.default_object_position_present=o.schema_default_object_position_present;
    out.default_object_position=o.schema_default_object_position;
    out.metadata_subblocks=o.schema_metadata_subblocks;
    out.block_size=o.schema_block_size;
    out.audio_bit_offset=delta?o.second_schema_audio_bit_offset:o.schema_audio_bit_offset;
    out.blob_bits=delta?o.second_schema_blob_bits:o.schema_blob_bits;
    out.consumed_bits=delta?o.second_schema_consumed_bits:o.schema_consumed_bits;
    out.config_header_bits=delta?o.second_schema_config_header_bits:o.schema_config_header_bits;
    out.programs_end_bit=delta?o.second_schema_programs_end_bit:o.schema_programs_end_bit;
    out.channel_end_bits=o.schema_channel_end_bits;
    out.pdu_count=o.schema_pdu_count;
    out.pdu_types_complete=o.schema_pdu_types_complete;
    out.pdu_types=o.schema_pdu_types;
    out.pdus=o.schema_pdus;
    out.primary_program_index=o.schema_primary_program_index;
    out.default_program_index=o.schema_default_program_index;
    out.programs=o.schema_program_data;
    out.bed_channels=o.schema_bed_channels;
    out.bed_channels_decoded=o.schema_bed_channels_decoded;
    out.beds=o.schema_bed_data;
    out.object_groups=o.schema_object_group_data;
    out.switch_groups=o.schema_switch_group_data;
}
}
bool auro_cx_declared_layout_from_acxd(
    const std::vector<std::uint8_t>& acxd,
    std::uint16_t& layout) {
    layout = 0u;
    // The configuration section is variable-length. Its fixed trailer is the
    // big-endian declared layout followed by one reserved zero byte. Known
    // encoder outputs use both 39-byte and extended 53-byte acxd payloads.
    if (acxd.size() < 39u || acxd.back() != 0u)
        return false;
    layout = be16(acxd.data() + acxd.size() - 3u);
    return layout != 0u;
}
bool probe_auro_cx_mp4(const std::string&path,AuroCxProbeInfo&o){
    o={};std::ifstream f(path,std::ios::binary);if(!f){o.error="cannot open input";return false;}std::vector<std::uint8_t>d((std::istreambuf_iterator<char>(f)),{});std::uint64_t c=0;
    while(c+8<=d.size()){Box r{};if(!next(d,c,d.size(),r))break;if(!is(r.type,"moov"))continue;std::uint64_t q=r.payload;while(q+8<=r.end){Box b{};if(!next(d,q,r.end,b))break;if(!is(b.type,"trak"))continue;Track t{};walk(d,b.payload,b.end,t);if(!t.audio||!t.cx)continue;build_sample_offsets(t);o.found=true;o.sample_entry="a3ds";o.container_channels=t.channels;o.sample_rate=t.rate;o.sample_count=t.count;o.samples_per_access_unit=t.duration;o.decoder_config=std::move(t.acxd);o.has_declared_layout=auro_cx_declared_layout_from_acxd(o.decoder_config,o.declared_layout);if(o.has_declared_layout){if(o.declared_layout==0x7FBF)o.declared_layout_name="7.1+5H+T (13.1)";else if(o.declared_layout==0x663F)o.declared_layout_name="5.1+4H (9.1)";else if(o.declared_layout==0x67BF)o.declared_layout_name="7.1+4H (11.1)";else if(o.declared_layout==0x01BF)o.declared_layout_name="7.1";}CxSchemaDecodeState schema_state{};if(!t.offsets.empty()&&!t.sizes.empty()){o.first_access_unit_offset=t.offsets[0];o.first_access_unit_size=t.sizes[0];auto x=o.first_access_unit_offset;o.first_access_unit_has_sync=x+4<=d.size()&&d[x]==0xA3&&d[x+1]==0xDC&&d[x+2]==0x0D&&d[x+3]==0xED;if(x+o.first_access_unit_size<=d.size())parse_segments(d.data()+x,d.data()+x+o.first_access_unit_size,o,&schema_state,false);}if(t.offsets.size()>1&&t.sizes.size()>1){o.second_access_unit_offset=t.offsets[1];o.second_access_unit_size=t.sizes[1];const auto x=o.second_access_unit_offset;o.second_access_unit_has_sync=x+4<=d.size()&&d[x]==0xA3&&d[x+1]==0xDC&&d[x+2]==0x0D&&d[x+3]==0xED;if(x+o.second_access_unit_size<=d.size()){AuroCxProbeInfo second{};parse_segments(d.data()+x,d.data()+x+o.second_access_unit_size,second,&schema_state,true);o.second_access_unit_segments=std::move(second.first_access_unit_segments);o.second_schema_pdu_count=second.schema_pdu_count;o.second_schema_pdu_types_complete=second.schema_pdu_types_complete;o.second_schema_pdu_types=std::move(second.schema_pdu_types);o.second_schema_pdus=std::move(second.schema_pdus);o.second_schema_consumed_bits=second.second_schema_consumed_bits;o.second_schema_blob_bits=second.second_schema_blob_bits;o.second_schema_audio_bit_offset=second.second_schema_audio_bit_offset;}}return true;}}
    o.error="OruaCX a3ds audio track not found";return false;
}
const char* auro_cx_awc_coding(const std::vector<AuroCxSchemaPduInfo>&pdus,std::size_t&awc_pdus){
    bool lossless=false,transparent=false;
    awc_pdus=0;
    for(const auto&pdu:pdus){
        if(pdu.type!=0)continue;
        ++awc_pdus;
        if(pdu.header_flag0)lossless=true;else transparent=true;
    }
    if(lossless&&transparent)return "mixed";
    if(lossless)return "lossless";
    if(transparent)return "transparent_near_lossless";
    return "unknown";
}
void print_auro_cx_probe(const AuroCxProbeInfo&i){
    std::cout<<"auro_cx_detected="<<(i.found?1:0)<<'\n';if(!i.found){std::cout<<"error="<<i.error<<'\n';return;}
    std::cout<<"container sample_entry="<<i.sample_entry<<" sample_rate="<<i.sample_rate<<" channels="<<i.container_channels<<" access_units="<<i.sample_count<<" samples_per_access_unit="<<i.samples_per_access_unit<<'\n';
    std::size_t awc_pdus=0;const char*awc_coding=auro_cx_awc_coding(i.schema_pdus,awc_pdus);std::cout<<"audio_coding awc="<<awc_coding<<" awc_pdus="<<awc_pdus<<'\n';
    std::cout<<"decoder_config box=acxd bytes="<<i.decoder_config.size()<<" data=";for(auto v:i.decoder_config)std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(v);std::cout<<std::dec<<'\n';
    std::cout<<"first_access_unit offset="<<i.first_access_unit_offset<<" bytes="<<i.first_access_unit_size<<" sync_a3dc0ded="<<(i.first_access_unit_has_sync?1:0)<<'\n';
    std::cout<<"blob_segments count="<<i.first_access_unit_segments.size();for(std::size_t n=0;n<i.first_access_unit_segments.size();++n)std::cout<<" segment"<<n<<"_id="<<i.first_access_unit_segments[n].identifier<<" segment"<<n<<"_bytes="<<i.first_access_unit_segments[n].payload_bytes;std::cout<<'\n';
    if(i.second_access_unit_size){std::cout<<"second_access_unit offset="<<i.second_access_unit_offset<<" bytes="<<i.second_access_unit_size<<" sync_a3dc0ded="<<(i.second_access_unit_has_sync?1:0)<<" blob_segments="<<i.second_access_unit_segments.size()<<" pdus="<<i.second_schema_pdu_count<<" decoded_types="<<i.second_schema_pdu_types.size()<<" complete="<<(i.second_schema_pdu_types_complete?1:0);if(i.second_schema_blob_bits)std::cout<<" schema_bits="<<i.second_schema_blob_bits<<" consumed_bits="<<i.second_schema_consumed_bits<<" audio_bit_offset="<<i.second_schema_audio_bit_offset<<" post_schema_bits="<<(i.second_schema_blob_bits>i.second_schema_consumed_bits?i.second_schema_blob_bits-i.second_schema_consumed_bits:0);std::cout<<" types=";static const char*second_types[]={"AWC","external","LFE","PCC","POC","ACC","SASC","EXT"};for(std::size_t n=0;n<i.second_schema_pdu_types.size();++n){if(n)std::cout<<',';const auto type=i.second_schema_pdu_types[n];std::cout<<(type<8?second_types[type]:"?");}std::cout<<'\n';for(std::size_t n=0;n<i.second_schema_pdus.size();++n){const auto&pdu=i.second_schema_pdus[n];std::cout<<"second_pdu index="<<n<<" type="<<pdu.type;if(pdu.payload_bits)std::cout<<" payload_offset="<<pdu.payload_bit_offset<<" payload_bits="<<pdu.payload_bits;std::cout<<'\n';}}
    std::cout<<"schema status="<<(i.schema_header_decoded?"header_decoded":"pending");if(i.schema_header_decoded)std::cout<<" codec_version="<<i.schema_codec_version<<" codec_profile="<<i.schema_codec_profile<<" programs="<<i.schema_programs<<" beds="<<i.schema_beds<<" object_groups="<<i.schema_object_groups<<" objects="<<(i.schema_object_groups?"?":"0")<<" switch_groups="<<i.schema_switch_groups<<" audio_bit_offset="<<i.schema_audio_bit_offset;else std::cout<<" programs=? beds=? object_groups=? objects=? switch_groups=?";
    if(i.has_declared_layout)std::cout<<" declared_layout=0x"<<std::hex<<i.declared_layout<<std::dec<<" layout=\""<<(i.declared_layout_name.empty()?"unknown":i.declared_layout_name)<<"\"";
    else std::cout<<" declared_layout=?";
    std::cout<<'\n';
    if(i.schema_header_decoded&&i.schema_programs)std::cout<<"program index=0 bed_references="<<i.schema_program0_bed_references<<"\n";
    if(i.schema_bed_channels_decoded){
        static const char*names[]={"FL","FR","C","LFE","LS","RS","CS","LB","RB","HL","HR","HC","T","HLS","HRS","HCS"};
        std::cout<<"bed index=0 type=channels source=schema channels="<<i.schema_bed_channels.size()<<" ids=";for(std::size_t n=0;n<i.schema_bed_channels.size();++n){if(n)std::cout<<',';const auto id=i.schema_bed_channels[n].id;std::cout<<(id<16?names[id]:"?");}std::cout<<'\n';
        std::cout<<"bed index=0 audio_stream_indices=";for(std::size_t n=0;n<i.schema_bed_channels.size();++n){if(n)std::cout<<',';std::cout<<i.schema_bed_channels[n].audio_stream_index;}std::cout<<'\n';
        std::cout<<"bed index=0 layers=";for(std::size_t n=0;n<i.schema_bed_channels.size();++n){if(n)std::cout<<',';std::cout<<i.schema_bed_channels[n].layer_index;}std::cout<<" downmix=";for(std::size_t n=0;n<i.schema_bed_channels.size();++n){if(n)std::cout<<',';std::cout<<(i.schema_bed_channels[n].downmix_present?1:0);}std::cout<<'\n';
        bool seen[32]={};unsigned streams=0;for(const auto&channel:i.schema_bed_channels)if(channel.audio_stream_index<32&&!seen[channel.audio_stream_index]){seen[channel.audio_stream_index]=true;++streams;}std::cout<<"audio_streams referenced="<<streams<<'\n';
    } else if(i.schema_header_decoded&&i.schema_beds==1&&i.schema_object_groups==0&&i.has_declared_layout){
        static const char*names[]={"FL","FR","C","LFE","LS","RS","CS","LB","RB","HL","HR","HC","T","HLS","HRS","HCS"};
        std::cout<<"bed index=0 type=channels source=declared_layout channels=";unsigned count=0;for(unsigned bit=0;bit<16;++bit)if((i.declared_layout>>bit)&1u)++count;std::cout<<count<<" ids=";bool comma=false;for(unsigned bit=0;bit<16;++bit)if((i.declared_layout>>bit)&1u){if(comma)std::cout<<',';std::cout<<names[bit];comma=true;}std::cout<<'\n';
    }
    for(std::size_t group=0;group<i.schema_object_group_data.size();++group){std::cout<<"object_group index="<<group<<" objects="<<i.schema_object_group_data[group].objects.size()<<" audio_stream_indices=";for(std::size_t object=0;object<i.schema_object_group_data[group].objects.size();++object){if(object)std::cout<<',';std::cout<<i.schema_object_group_data[group].objects[object].audio_stream_index;}std::cout<<'\n';}
    for(std::size_t group=0;group<i.schema_switch_group_data.size();++group)std::cout<<"switch_group index="<<group<<" elements="<<i.schema_switch_group_data[group].elements.size()<<'\n';
    if(i.schema_pdu_count){static const char*types[]={"AWC","external","LFE","PCC","POC","ACC","SASC","EXT"};std::cout<<"pdus declared="<<i.schema_pdu_count<<" decoded_types="<<i.schema_pdu_types.size()<<" complete="<<(i.schema_pdu_types_complete?1:0);if(i.schema_blob_bits)std::cout<<" schema_bits="<<i.schema_blob_bits<<" consumed_bits="<<i.schema_consumed_bits<<" post_schema_bits="<<(i.schema_blob_bits>i.schema_consumed_bits?i.schema_blob_bits-i.schema_consumed_bits:0);std::cout<<" types=";for(std::size_t n=0;n<i.schema_pdu_types.size();++n){if(n)std::cout<<',';const auto type=i.schema_pdu_types[n];std::cout<<(type<8?types[type]:"?");}std::cout<<'\n';for(std::size_t n=0;n<i.schema_pdus.size();++n){const auto&pdu=i.schema_pdus[n];std::cout<<"pdu index="<<n<<" type="<<(pdu.type<8?types[pdu.type]:"?");if(pdu.audio_stream_count)std::cout<<" first_audio_stream="<<pdu.first_audio_stream<<" audio_stream_count="<<pdu.audio_stream_count;if(pdu.type==0)std::cout<<" coding="<<(pdu.header_flag0?"lossless":"transparent_near_lossless")<<" lossless="<<pdu.header_flag0<<" header_flag1="<<pdu.header_flag1<<" header_value="<<pdu.header_value;if(pdu.type==2)std::cout<<" header_flag0="<<pdu.header_flag0<<" header_value="<<pdu.header_value;if(pdu.type==3||pdu.type==4||pdu.type==5){std::cout<<" bed_index="<<pdu.transform_bed_index<<" source_layer="<<pdu.transform_source_layer<<" target_layer="<<pdu.transform_target_layer;if(pdu.type==5)std::cout<<" resample_factor="<<pdu.transform_resample_factor;}if(pdu.type==6){std::cout<<" element_description="<<(pdu.header_value==0?"object_group":pdu.header_value==1?"channel_bed":pdu.header_value==2?"bed":"unknown")<<" element_description_type="<<pdu.header_value<<" resample_factor="<<pdu.sasc_resample_factor<<" scg_levels="<<pdu.sasc_channel_sets.size();if(pdu.header_value==0)std::cout<<" object_group_index="<<pdu.sasc_object_group_index;if(pdu.header_value==1)std::cout<<" bed_index="<<pdu.sasc_bed_index<<" source_layer="<<pdu.sasc_channel_bed_layer<<" config_flag="<<(pdu.sasc_config_flag?1:0)<<" mode_flag="<<(pdu.sasc_mode_flag?1:0);for(std::size_t level=0;level<pdu.sasc_channel_sets.size();++level){std::size_t enabled=0;for(const bool bit:pdu.sasc_channel_sets[level])if(bit)++enabled;std::cout<<" scg_level"<<level<<"_enabled="<<enabled;}}if(pdu.payload_bits){std::cout<<" payload_bits="<<pdu.payload_bits<<" payload_data=";for(const auto byte:pdu.payload_data)std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(byte);std::cout<<std::dec;}if(pdu.awc_payload_config_decoded){std::cout<<" awc_common_preamble="<<pdu.awc_common_preamble<<" awc_stream_parameters=";for(std::size_t stream=0;stream<pdu.awc_stream_parameters.size();++stream){if(stream)std::cout<<',';std::cout<<pdu.awc_stream_parameters[stream];}}std::cout<<'\n';}}
}
bool cx_parse_schema_blob(const std::vector<std::uint8_t>&blob,CxSchemaParseResult&out,CxSchemaDecodeState*state,bool delta){
    AuroCxProbeInfo probe{};
    if(delta){probe.second_schema_blob_bits=static_cast<std::uint32_t>(blob.size()*8);}else{probe.schema_blob_bits=static_cast<std::uint32_t>(blob.size()*8);}
    const bool ok=parse_schema_header(blob,probe,state,delta);
    fill_schema_result(probe,out,delta);
    out.blob_bits=static_cast<std::uint32_t>(blob.size()*8);
    return ok&&out.header_decoded;
}
bool cx_decode_xor_segment1(const std::uint8_t*begin,const std::uint8_t*end,std::vector<std::uint8_t>&decoded,std::string&error){
    decoded.clear();
    if(end-begin<8||begin[0]!=0xA3||begin[1]!=0xDC||begin[2]!=0x0D||begin[3]!=0xED){error="missing A3DC0DED sync";return false;}
    const std::uint8_t*key=begin+4;const std::uint8_t*p=begin+8;unsigned key_pos=0;std::uint32_t id=0,len=0;
    if(!xor_varint(p,end,key,key_pos,id)||!xor_varint(p,end,key,key_pos,len)){error="segment varint decode failed";return false;}
    if(id!=1){error="expected XOR segment id 1";return false;}
    if(std::uint64_t(end-p)<len){error="segment payload truncated";return false;}
    decoded.resize(len);
    for(std::uint32_t i=0;i<len;++i)decoded[i]=p[i]^key[(key_pos+i)&3];
    return true;
}
bool cx_read_config_block_size(const std::vector<std::uint8_t>&blob,bool delta,const CxSchemaDecodeState*state,std::uint32_t&block_size){
    AuroCxProbeInfo probe{};
    if(!parse_schema_header(blob,probe,const_cast<CxSchemaDecodeState*>(state),delta)||!probe.schema_block_size)
        return false;
    block_size=probe.schema_block_size;
    return true;
}
}
