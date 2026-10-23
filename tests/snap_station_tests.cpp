#include "snap_station.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace ss=isle::snap_station;
namespace fs=std::filesystem;
namespace {
void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
struct OutputRoot {
    fs::path path;
    OutputRoot() {
        static unsigned sequence=0;
        const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
        for(unsigned attempt=0;attempt<100;++attempt) {
            auto candidate=fs::temp_directory_path()/
                ("35mmisle-station-"+std::to_string(stamp)+"-"+std::to_string(++sequence));
            if(fs::create_directory(candidate)) {path=std::move(candidate);break;}
        }
        require(!path.empty(),"cannot acquire isolated output root");
    }
    ~OutputRoot() {std::error_code ignored;fs::remove_all(path,ignored);}
};
std::vector<unsigned char> bytes(const fs::path& path) {
    std::ifstream input(path,std::ios::binary);
    require(bool(input),"published sheet cannot be opened");
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
std::uint32_t little32(const std::vector<unsigned char>& data,std::size_t offset) {
    return std::uint32_t(data.at(offset))|(std::uint32_t(data.at(offset+1))<<8)|
        (std::uint32_t(data.at(offset+2))<<16)|(std::uint32_t(data.at(offset+3))<<24);
}
void actual_sheet_and_reset() {
    OutputRoot root;
    ss::Device device;
    require(device.connect(root.path),device.message());
    ss::Memory memory;
    require(device.memory(memory),device.message());
    auto* words=static_cast<volatile std::uint32_t*>(memory.words);
    for(std::size_t i=memory.word_count;i<memory.byte_count/4;++i)words[i]=0x13579bdfu;
    require(device.cache(memory,ss::CacheOperation::writeback),device.message());
    for(const std::uint32_t pattern:{0x55555555u,0xaaaaaaaau}) {
        for(std::size_t i=0;i<memory.word_count;++i)words[i]=pattern;
        require(device.cache(memory,ss::CacheOperation::invalidate),device.message());
        for(std::size_t i=0;i<memory.word_count;++i)
            require(words[i]==pattern,"workspace lost actual pattern writes");
        for(std::size_t i=memory.word_count;i<memory.byte_count/4;++i)
            require(words[i]==0x13579bdfu,"pattern loop overwrote excluded tail");
    }
    require(device.command(0xcc),device.message());
    require(device.command(0x33),device.message());
    require(device.command(0x5a),device.message());
    std::uint8_t status=0;
    for(unsigned poll=0;poll<3;++poll)
        require(device.status(status)&&status==8,"status polling completed an unconsumed reset");
    std::uint64_t ticket=0,duplicate=0;
    require(device.take_reset_request(ticket)&&ticket,"reset operation has no ticket");
    require(!device.take_reset_request(duplicate),"one reset operation was consumed twice");
    // Backend client fixture only; this does not qualify an application reboot.
    require(device.complete_reset(2),device.message());
    std::vector<std::byte> rgba(640*480*4);
    int owner=0;
    for(unsigned pass=0;pass<16;++pass) {
        require(device.begin_pass(2,pass+1,&owner,pass),device.message());
        if(pass==0)require(device.command(1),device.message());
        for(std::size_t i=0;i<rgba.size();i+=4) {
            rgba[i]=std::byte(pass*13);rgba[i+1]=std::byte(255-pass*11);
            rgba[i+2]=std::byte(pass*7);rgba[i+3]=std::byte{255};
        }
        require(device.stage_frame({2,pass+1,1,100+pass,&owner,pass},rgba,640,480,2560),device.message());
        require(device.command(2),device.message());
    }
    require(device.begin_pass(2,17,&owner,16),device.message());
    require(device.command(4),device.message());
    require(device.command(4),device.message());
    std::vector<fs::path> sheets;
    for(const auto& entry:fs::directory_iterator(device.output_directory()))
        if(entry.path().extension()==".bmp")sheets.push_back(entry.path());
    require(sheets.size()==2,"terminal print did not publish one sheet and one repeat");
    const auto first=bytes(sheets[0]);
    require(first==bytes(sheets[1]),"terminal repeat changed the completed sheet");
    require(first.size()==14745654&&first[0]=='B'&&first[1]=='M',"invalid BMP publication extent");
    require(little32(first,18)==2560&&little32(first,22)==1920,"wrong sheet dimensions");
    for(unsigned tile=0;tile<16;++tile) {
        for(const unsigned corner:{0u,1u,2u,3u}) {
            const auto x=(tile%4)*640+(corner&1u?639u:0u);
            const auto y=(tile/4)*480+(corner&2u?479u:0u);
            const std::size_t offset=54+std::size_t(1919-y)*2560*3+x*3;
            require(first.at(offset)==tile*7&&first.at(offset+1)==255-tile*11&&
                first.at(offset+2)==tile*13,"capture region was flipped, truncated or placed in the wrong tile");
        }
    }
}
void reject_wrong_and_missing_frames() {
    OutputRoot root;
    std::vector<std::byte> rgba(640*480*4);
    int owner=0,foreign_owner=0;
    {
        ss::Device device;require(device.connect(root.path),device.message());
        ss::Memory memory;require(device.memory(memory),device.message());++memory.words;
        require(!device.cache(memory,ss::CacheOperation::invalidate),"foreign cache pointer accepted");
        require(!device.available(),"failed cache descriptor left usable capability");
    }
    {
        ss::Device device;require(device.connect(root.path),device.message());
        require(device.begin_pass(1,1,&owner,0)&&device.command(1),device.message());
        require(device.stage_frame({1,1,1,1,&owner,0},rgba,640,480,2560),device.message());
        require(!device.stage_frame({1,1,1,1,&owner,0},rgba,640,480,2560),"stale completed ordinal accepted");
        std::uint8_t status=0;
        require(!device.status(status),"failed frame staging produced a successful status");
    }
    {
        ss::Device device;require(device.connect(root.path),device.message());
        require(device.begin_pass(1,1,&owner,0),device.message());
        require(!device.stage_frame({1,1,1,1,&foreign_owner,0},rgba,640,480,2560),"foreign Frame owner accepted");
    }
    {
        ss::Device device;require(device.connect(root.path),device.message());
        require(device.begin_pass(1,1,&owner,0)&&device.command(1),device.message());
        require(!device.command(2),"capture without a completed frame acknowledged");
    }
    for(const auto& job:fs::directory_iterator(root.path))
        require(job.is_directory()&&fs::is_empty(job.path()),"rejected capture published or leaked an owned partial");
}
}
int main()try {
    actual_sheet_and_reset();
    reject_wrong_and_missing_frames();
    std::cout<<"actual station publication and ownership boundaries passed\n";
    return 0;
}catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';return 1;
}
