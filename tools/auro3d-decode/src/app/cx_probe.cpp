#include "cx_probe.hpp"
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
    if(is(b.type,"hdlr")&&b.payload+12<=b.end)t.audio=is(d.data()+b.payload+8,"soun");
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
    value=0;for(unsigned n=0;n<5&&p<end;++n,++p,++key_pos){const std::uint8_t byte=*p^key[key_pos&3];value=(value<<7)|(byte&0x7f);if((byte&0x80)==0){++p;++key_pos;return true;}}return false;
}
struct Bits { const std::vector<std::uint8_t>&d;std::size_t p=0;bool get(unsigned n,std::uint32_t&v){if(p+n>d.size()*8||n>32)return false;v=0;for(unsigned i=0;i<n;++i)v|=((d[(p+i)/8]>>((p+i)&7))&1u)<<i;p+=n;return true;}bool unary(std::uint32_t&v){v=0;std::uint32_t x=0;while(get(1,x)){if(!x)return true;++v;}return false;} };
unsigned index_bits(std::uint32_t maximum){unsigned n=0;while(maximum){++n;maximum>>=1;}return n;}
bool skip_bits(Bits&b,std::uint64_t count){std::uint32_t ignored=0;while(count){const unsigned chunk=static_cast<unsigned>(count>32?32:count);if(!b.get(chunk,ignored))return false;count-=chunk;}return true;}
std::size_t find_pdu_vector_start(const std::vector<std::uint8_t>&d,std::size_t first,unsigned audio_stream_bits){
    std::size_t best=d.size()*8;std::uint32_t best_count=0;
    for(std::size_t start=first;start<d.size()*8;++start){Bits b{d};b.p=start;std::uint32_t q=0,r=0;if(!b.unary(q)||q>16||!b.get(audio_stream_bits,r))continue;const std::uint32_t count=(q<<audio_stream_bits)|r;if(!count||count>16)continue;bool valid=true,ends_in_sasc=false;
        for(std::uint32_t n=0;n<count&&valid;++n){std::uint32_t type=0,tmp=0;if(!b.get(3,type)){valid=false;break;}if(type==0){if(!b.get(audio_stream_bits,tmp)||!b.get(1,tmp)||!b.get(1,tmp)){valid=false;break;}if(tmp&&!b.get(1,tmp)){valid=false;break;}}else if(type==1){if(!b.get(audio_stream_bits,tmp)){valid=false;break;}}else if(type==2){if(!b.get(audio_stream_bits,tmp)||!b.get(1,tmp)){valid=false;break;}if(tmp&&!b.get(3,tmp)){valid=false;break;}}else if(type==6){if(!b.get(2,tmp)||tmp==3||n+1!=count){valid=false;break;}ends_in_sasc=true;continue;}else{valid=false;break;}if(type<=2){if(!b.unary(q)||q>0x100000||!b.get(8,r)||!skip_bits(b,(std::uint64_t(q)<<8)+r+1)){valid=false;break;}}}
        if(valid&&ends_in_sasc&&(count>best_count||(count==best_count&&start<best))){best=start;best_count=count;}
    }return best;
}
bool parse_schema_header(const std::vector<std::uint8_t>&d,AuroCxProbeInfo&o){
    Bits b{d};std::uint32_t v=0,profile=0,sample_rate_id=0,flag=0,block_id=0,tmp=0;
    if(!b.unary(v)||!b.unary(profile)||!b.get(3,sample_rate_id))return false;if(sample_rate_id==7&&!b.get(20,tmp))return false;
    if(!b.get(1,flag)||!b.get(5,block_id))return false;if(block_id==31){if(!b.get(12,tmp))return false;}else if(block_id>=24)return false;
    if(!b.get(3,tmp)||!b.get(1,flag))return false;if(flag&&!b.get(12,tmp))return false;
    std::uint32_t audio_stream_bits=0;
    if(!b.unary(audio_stream_bits)||!b.unary(tmp)||!b.get(1,flag))return false;if(flag&&!b.get(16,tmp))return false;
    std::uint32_t common=0;if(!b.get(1,common)||common)return false;std::uint32_t gains_enabled=0,explicit_metadata=0;if(!b.get(1,gains_enabled)||!b.get(1,explicit_metadata))return false;
    std::uint32_t programs=1,beds=0,objects=0,switches=0;if(explicit_metadata&&!b.unary(programs))return false;if(explicit_metadata)++programs;
    if(!b.unary(beds)||!b.unary(objects))return false;if(explicit_metadata&&!b.unary(switches))return false;
    o.schema_header_decoded=true;o.schema_codec_version=v;o.schema_codec_profile=profile;o.schema_programs=programs;o.schema_beds=beds;o.schema_object_groups=objects;o.schema_switch_groups=switches;
    const unsigned program_index_bits=index_bits(programs-1);if(!b.get(program_index_bits,tmp)||!b.get(program_index_bits,tmp))return true;
    std::uint32_t extensions=0;if(!b.unary(extensions))return true;
    for(std::uint32_t program=0;program<programs;++program){
        if(!b.get(1,flag))return true;if(flag&&!b.unary(tmp))return true;
        if(explicit_metadata){if(!b.get(1,flag)||flag)return true;}
        if(!b.get(1,flag))return true;if(flag&&!b.get(4,tmp))return true;if(!b.get(1,flag))return true;
        std::uint32_t refs=0;if(!b.get(index_bits(beds),refs))return true;if(program==0)o.schema_program0_bed_references=refs;
        for(std::uint32_t n=0;n<refs;++n){if(explicit_metadata&&!b.get(1,tmp))return true;if(gains_enabled){if(!b.get(2,tmp))return true;if(tmp>=2&&!b.get(10,tmp))return true;}if(!b.get(index_bits(beds-1),tmp))return true;}
        if(!b.get(index_bits(objects),refs))return true;
        for(std::uint32_t n=0;n<refs;++n){if(explicit_metadata&&!b.get(1,tmp))return true;if(gains_enabled){if(!b.get(2,tmp))return true;if(tmp>=2&&!b.get(10,tmp))return true;}if(!b.get(index_bits(objects-1),tmp))return true;}
        if(!b.get(index_bits(switches),refs))return true;for(std::uint32_t n=0;n<refs;++n)if(!b.get(index_bits(switches-1),tmp))return true;
        if(!b.get(1,flag)||flag)return true;if(!b.get(1,flag)||flag)return true;
        if(!b.get(1,flag))return true;if(flag){if(!b.get(1,tmp)||!b.get(2,tmp)||tmp==3)return true;}
        if(!b.get(1,flag))return true;if(flag&&!b.get(index_bits(beds-1),tmp))return true;
    }
    if(objects||switches||extensions)return true;
    for(std::uint32_t bed=0;bed<beds;++bed){
        if(explicit_metadata){if(!b.get(1,flag)||flag)return true;if(!b.get(1,flag))return true;if(flag){if(!b.get(1,tmp)||!b.get(5,tmp)||!b.get(4,tmp))return true;}}
        if(!b.get(1,flag))return true;if(flag&&!b.get(1,tmp))return true;
        std::uint32_t ambisonics=0;if(!b.get(1,ambisonics)||ambisonics)return true;
        std::uint32_t layers=0;if(!b.unary(layers))return true;++layers;if(layers>4)return true;
        if(!b.get(1,flag))return true;if(flag&&layers!=1&&!b.get(2,tmp))return true;
        std::uint32_t channels=0;if(!b.get(6,channels))return true;
        const std::size_t channel_descriptors_start=b.p;
        bool channel_descriptors_valid=true;
        std::vector<AuroCxSchemaChannelInfo> parsed;parsed.reserve(channels);
        for(std::uint32_t n=0;n<channels;++n){
            std::uint32_t groups=0,low=0,stream=0;
            if(!b.unary(groups)||!b.get(2,low)||!b.get(audio_stream_bits,stream)){channel_descriptors_valid=false;break;}
            if(layers>1&&!b.get(index_bits(layers-1),tmp)){channel_descriptors_valid=false;break;}
            if(gains_enabled){if(!b.get(2,tmp)){channel_descriptors_valid=false;break;}if(tmp>=2&&!b.get(10,tmp)){channel_descriptors_valid=false;break;}}
            if(!b.get(1,flag)||!b.get(1,flag)||flag){channel_descriptors_valid=false;break;}
            parsed.push_back({4*groups+low,stream});
        }
        if(!channel_descriptors_valid){const auto pdu_start=find_pdu_vector_start(d,channel_descriptors_start,audio_stream_bits);b.p=pdu_start<d.size()*8?pdu_start:channel_descriptors_start;parsed.clear();}
        if(bed==0&&channel_descriptors_valid){o.schema_bed_channels=std::move(parsed);o.schema_bed_channels_decoded=true;}
    }
    o.schema_audio_bit_offset=static_cast<std::uint32_t>(b.p);
    std::uint32_t quotient=0,remainder=0;
    if(!b.unary(quotient)||!b.get(audio_stream_bits,remainder))return true;
    o.schema_pdu_count=(quotient<<audio_stream_bits)|remainder;
    std::uint32_t next_audio_stream=0;
    for(std::uint32_t n=0;n<o.schema_pdu_count;++n){
        std::uint32_t type=0;if(!b.get(3,type))return true;o.schema_pdu_types.push_back(type);
        AuroCxSchemaPduInfo pdu{};pdu.type=type;
        bool supported=true;
        if(type==0){if(!b.get(audio_stream_bits,tmp))return true;pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;if(!b.get(1,pdu.header_flag0)||!b.get(1,pdu.header_flag1))return true;if(pdu.header_flag1&&!b.get(1,pdu.header_value))return true;}
        else if(type==1){if(!b.get(audio_stream_bits,tmp))return true;pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;}
        else if(type==2){if(!b.get(audio_stream_bits,tmp))return true;pdu.first_audio_stream=next_audio_stream;pdu.audio_stream_count=tmp+1;next_audio_stream+=pdu.audio_stream_count;if(!b.get(1,pdu.header_flag0))return true;if(pdu.header_flag0&&!b.get(3,pdu.header_value))return true;}
        else if(type==6){if(!b.get(2,pdu.header_value)||pdu.header_value==3)return true;supported=false;}
        else supported=false;
        if(type<=5){std::uint32_t q=0,r=0;if(!supported||!b.unary(q)||!b.get(8,r))return true;pdu.payload_bits=((std::uint64_t(q)<<8)|r)+1;pdu.payload_data.assign(static_cast<std::size_t>((pdu.payload_bits+7)/8),0);for(std::uint64_t bit=0;bit<pdu.payload_bits;++bit){if(!b.get(1,tmp))return true;pdu.payload_data[static_cast<std::size_t>(bit/8)]|=static_cast<std::uint8_t>(tmp<<(bit&7));}if(type==0){const unsigned common_bits=pdu.header_flag0?9u:8u;const unsigned per_stream_bits=pdu.header_flag0?4u:12u;if(pdu.payload_bits==common_bits+std::uint64_t(per_stream_bits)*pdu.audio_stream_count){Bits payload{pdu.payload_data};if(payload.get(common_bits,pdu.awc_common_preamble)){pdu.awc_payload_config_decoded=true;for(std::uint32_t stream=0;stream<pdu.audio_stream_count;++stream){std::uint32_t parameter=0;if(!payload.get(per_stream_bits,parameter)){pdu.awc_payload_config_decoded=false;break;}pdu.awc_stream_parameters.push_back(parameter);}}}}}
        o.schema_pdus.push_back(pdu);
        if(type>5&&!supported){if(n+1==o.schema_pdu_count)o.schema_pdu_types_complete=true;return true;}
    }
    o.schema_pdu_types_complete=true;
    return true;
}
void parse_segments(const std::uint8_t*p,const std::uint8_t*end,AuroCxProbeInfo&o){
    while(end-p>=8&&p[0]==0xA3&&p[1]==0xDC&&p[2]==0x0D&&p[3]==0xED){const std::uint8_t*key=p+4;const std::uint8_t*q=p+8;unsigned key_pos=0;std::uint32_t id=0,len=0;if(!xor_varint(q,end,key,key_pos,id)||!xor_varint(q,end,key,key_pos,len)||std::uint64_t(end-q)<len)break;o.first_access_unit_segments.push_back({id,len});if(id==1){std::vector<std::uint8_t>decoded(len);for(std::uint32_t i=0;i<len;++i)decoded[i]=q[i]^key[(key_pos+i)&3];parse_schema_header(decoded,o);}p=q+len;}
}
}
bool probe_auro_cx_mp4(const std::string&path,AuroCxProbeInfo&o){
    o={};std::ifstream f(path,std::ios::binary);if(!f){o.error="cannot open input";return false;}std::vector<std::uint8_t>d((std::istreambuf_iterator<char>(f)),{});std::uint64_t c=0;
    while(c+8<=d.size()){Box r{};if(!next(d,c,d.size(),r))break;if(!is(r.type,"moov"))continue;std::uint64_t q=r.payload;while(q+8<=r.end){Box b{};if(!next(d,q,r.end,b))break;if(!is(b.type,"trak"))continue;Track t{};walk(d,b.payload,b.end,t);if(!t.audio||!t.cx)continue;build_sample_offsets(t);o.found=true;o.sample_entry="a3ds";o.container_channels=t.channels;o.sample_rate=t.rate;o.sample_count=t.count;o.samples_per_access_unit=t.duration;o.decoder_config=std::move(t.acxd);if(o.decoder_config.size()>=38){o.has_declared_layout=true;o.declared_layout=be16(o.decoder_config.data()+36);if(o.declared_layout==0x7FBF)o.declared_layout_name="7.1+5H+T (13.1)";else if(o.declared_layout==0x663F)o.declared_layout_name="5.1+4H (9.1)";else if(o.declared_layout==0x01BF)o.declared_layout_name="7.1";}if(!t.offsets.empty()&&!t.sizes.empty()){o.first_access_unit_offset=t.offsets[0];o.first_access_unit_size=t.sizes[0];auto x=o.first_access_unit_offset;o.first_access_unit_has_sync=x+4<=d.size()&&d[x]==0xA3&&d[x+1]==0xDC&&d[x+2]==0x0D&&d[x+3]==0xED;if(x+o.first_access_unit_size<=d.size())parse_segments(d.data()+x,d.data()+x+o.first_access_unit_size,o);}if(t.offsets.size()>1&&t.sizes.size()>1){o.second_access_unit_offset=t.offsets[1];o.second_access_unit_size=t.sizes[1];const auto x=o.second_access_unit_offset;o.second_access_unit_has_sync=x+4<=d.size()&&d[x]==0xA3&&d[x+1]==0xDC&&d[x+2]==0x0D&&d[x+3]==0xED;if(x+o.second_access_unit_size<=d.size()){AuroCxProbeInfo second{};parse_segments(d.data()+x,d.data()+x+o.second_access_unit_size,second);o.second_access_unit_segments=std::move(second.first_access_unit_segments);o.second_schema_pdu_count=second.schema_pdu_count;o.second_schema_pdu_types_complete=second.schema_pdu_types_complete;o.second_schema_pdu_types=std::move(second.schema_pdu_types);o.second_schema_pdus=std::move(second.schema_pdus);}}return true;}}
    o.error="AuroCX a3ds audio track not found";return false;
}
void print_auro_cx_probe(const AuroCxProbeInfo&i){
    std::cout<<"auro_cx_detected="<<(i.found?1:0)<<'\n';if(!i.found){std::cout<<"error="<<i.error<<'\n';return;}
    std::cout<<"container sample_entry="<<i.sample_entry<<" sample_rate="<<i.sample_rate<<" channels="<<i.container_channels<<" access_units="<<i.sample_count<<" samples_per_access_unit="<<i.samples_per_access_unit<<'\n';
    std::cout<<"decoder_config box=acxd bytes="<<i.decoder_config.size()<<" data=";for(auto v:i.decoder_config)std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(v);std::cout<<std::dec<<'\n';
    std::cout<<"first_access_unit offset="<<i.first_access_unit_offset<<" bytes="<<i.first_access_unit_size<<" sync_a3dc0ded="<<(i.first_access_unit_has_sync?1:0)<<'\n';
    std::cout<<"blob_segments count="<<i.first_access_unit_segments.size();for(std::size_t n=0;n<i.first_access_unit_segments.size();++n)std::cout<<" segment"<<n<<"_id="<<i.first_access_unit_segments[n].identifier<<" segment"<<n<<"_bytes="<<i.first_access_unit_segments[n].payload_bytes;std::cout<<'\n';
    if(i.second_access_unit_size){std::cout<<"second_access_unit offset="<<i.second_access_unit_offset<<" bytes="<<i.second_access_unit_size<<" sync_a3dc0ded="<<(i.second_access_unit_has_sync?1:0)<<" blob_segments="<<i.second_access_unit_segments.size()<<" pdus="<<i.second_schema_pdu_count<<" decoded_types="<<i.second_schema_pdu_types.size()<<" complete="<<(i.second_schema_pdu_types_complete?1:0)<<" types=";static const char*second_types[]={"AWC","external","LFE","PCC","POC","ACC","SASC","EXT"};for(std::size_t n=0;n<i.second_schema_pdu_types.size();++n){if(n)std::cout<<',';const auto type=i.second_schema_pdu_types[n];std::cout<<(type<8?second_types[type]:"?");}std::cout<<'\n';}
    std::cout<<"schema status="<<(i.schema_header_decoded?"header_decoded":"pending");if(i.schema_header_decoded)std::cout<<" codec_version="<<i.schema_codec_version<<" codec_profile="<<i.schema_codec_profile<<" programs="<<i.schema_programs<<" beds="<<i.schema_beds<<" object_groups="<<i.schema_object_groups<<" objects="<<(i.schema_object_groups?"?":"0")<<" switch_groups="<<i.schema_switch_groups;else std::cout<<" programs=? beds=? object_groups=? objects=? switch_groups=?";
    if(i.has_declared_layout)std::cout<<" declared_layout=0x"<<std::hex<<i.declared_layout<<std::dec<<" layout=\""<<(i.declared_layout_name.empty()?"unknown":i.declared_layout_name)<<"\"";
    else std::cout<<" declared_layout=?";
    std::cout<<'\n';
    if(i.schema_header_decoded&&i.schema_programs)std::cout<<"program index=0 bed_references="<<i.schema_program0_bed_references<<"\n";
    if(i.schema_bed_channels_decoded){
        static const char*names[]={"FL","FR","C","LFE","LS","RS","CS","LB","RB","HL","HR","HC","T","HLS","HRS","HCS"};
        std::cout<<"bed index=0 type=channels source=schema channels="<<i.schema_bed_channels.size()<<" ids=";for(std::size_t n=0;n<i.schema_bed_channels.size();++n){if(n)std::cout<<',';const auto id=i.schema_bed_channels[n].id;std::cout<<(id<16?names[id]:"?");}std::cout<<'\n';
        std::cout<<"bed index=0 audio_stream_indices=";for(std::size_t n=0;n<i.schema_bed_channels.size();++n){if(n)std::cout<<',';std::cout<<i.schema_bed_channels[n].audio_stream_index;}std::cout<<'\n';
        bool seen[32]={};unsigned streams=0;for(const auto&channel:i.schema_bed_channels)if(channel.audio_stream_index<32&&!seen[channel.audio_stream_index]){seen[channel.audio_stream_index]=true;++streams;}std::cout<<"audio_streams referenced="<<streams<<'\n';
    } else if(i.schema_header_decoded&&i.schema_beds==1&&i.schema_object_groups==0&&i.has_declared_layout){
        static const char*names[]={"FL","FR","C","LFE","LS","RS","CS","LB","RB","HL","HR","HC","T","HLS","HRS","HCS"};
        std::cout<<"bed index=0 type=channels source=declared_layout channels=";unsigned count=0;for(unsigned bit=0;bit<16;++bit)if((i.declared_layout>>bit)&1u)++count;std::cout<<count<<" ids=";bool comma=false;for(unsigned bit=0;bit<16;++bit)if((i.declared_layout>>bit)&1u){if(comma)std::cout<<',';std::cout<<names[bit];comma=true;}std::cout<<'\n';
    }
    if(i.schema_pdu_count){static const char*types[]={"AWC","external","LFE","PCC","POC","ACC","SASC","EXT"};std::cout<<"pdus declared="<<i.schema_pdu_count<<" decoded_types="<<i.schema_pdu_types.size()<<" complete="<<(i.schema_pdu_types_complete?1:0)<<" types=";for(std::size_t n=0;n<i.schema_pdu_types.size();++n){if(n)std::cout<<',';const auto type=i.schema_pdu_types[n];std::cout<<(type<8?types[type]:"?");}std::cout<<'\n';for(std::size_t n=0;n<i.schema_pdus.size();++n){const auto&pdu=i.schema_pdus[n];std::cout<<"pdu index="<<n<<" type="<<(pdu.type<8?types[pdu.type]:"?");if(pdu.audio_stream_count)std::cout<<" first_audio_stream="<<pdu.first_audio_stream<<" audio_stream_count="<<pdu.audio_stream_count;if(pdu.type==0)std::cout<<" lossless="<<pdu.header_flag0<<" header_flag1="<<pdu.header_flag1<<" header_value="<<pdu.header_value;if(pdu.type==2)std::cout<<" header_flag0="<<pdu.header_flag0<<" header_value="<<pdu.header_value;if(pdu.type==6)std::cout<<" element_description="<<(pdu.header_value==0?"object_group":pdu.header_value==1?"channel_bed":pdu.header_value==2?"bed":"unknown")<<" element_description_type="<<pdu.header_value;if(pdu.payload_bits){std::cout<<" payload_bits="<<pdu.payload_bits<<" payload_data=";for(const auto byte:pdu.payload_data)std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(byte);std::cout<<std::dec;}if(pdu.awc_payload_config_decoded){std::cout<<" awc_common_preamble="<<pdu.awc_common_preamble<<" awc_stream_parameters=";for(std::size_t stream=0;stream<pdu.awc_stream_parameters.size();++stream){if(stream)std::cout<<',';std::cout<<pdu.awc_stream_parameters[stream];}}std::cout<<'\n';}}
}
}
