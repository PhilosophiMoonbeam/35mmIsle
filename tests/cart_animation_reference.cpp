#include "cart_animation_reference.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace isle::player::reference {
namespace {
void require(bool valid) {
    if (!valid) throw std::runtime_error("Unadmitted cart numeric reference DATA");
}
class Data {
    std::span<const std::byte> bytes_;
public:
    explicit Data(const rom::Snapshot& snapshot) : bytes_(snapshot.canonical()) {
        constexpr std::array<std::uint8_t,20> identity{
            0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
            0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
        require(snapshot.identity().size==rom::us_size && bytes_.size()==rom::us_size &&
                snapshot.identity().sha1==identity);
    }
    std::uint32_t word(std::uint32_t address) const {
        constexpr std::uint32_t base=0x8011b050, extent=0x952a0;
        require(address>=base && address-base<=extent-4 && (address&3)==0);
        const auto offset=0xa74e0u+(address-base);
        std::uint32_t value=0;
        for (unsigned i=0;i<4;++i) value=(value<<8)|std::to_integer<unsigned>(bytes_[offset+i]);
        return value;
    }
    float scalar(std::uint32_t address) const {
        const auto result=std::bit_cast<float>(word(address));
        require(std::isfinite(result));
        return result;
    }
};
}
float camera_cubic(float initial,float target,float rate,float target_rate,float duration,float time) {
    require(duration>0 && std::isfinite(duration) && std::isfinite(time));
    return isle_reference_camera(1.0f/duration,time,initial,target,rate,target_rate);
}
float model_cubic(float initial,float target,float rate,float target_rate,float duration,float time) {
    require(duration>0 && std::isfinite(duration) && std::isfinite(time));
    return isle_reference_model(1.0f/duration,time,initial,target,rate,target_rate);
}
CartNumericReference::CartNumericReference(const rom::Snapshot& snapshot) {
    const Data data(snapshot);
    constexpr std::array<std::uint32_t,4> bindings{0x8013b140,0x8013c530,0x8013c6b4,0x8013da90};
    std::size_t slot=0;
    for (std::size_t binding=0;binding<bindings.size();++binding) {
        const bool camera=(binding&1)!=0;
        const auto address=bindings[binding];
        const auto command=data.word(address);
        require(command>>25==13 && ((command>>15)&1023u)==(camera?136u:8u) && (command&32767u)==0);
        for (unsigned channel=0;channel<(camera?2u:1u);++channel) {
            auto& path=paths_[slot++];
            const auto descriptor=data.word(address+4+channel*4);
            const auto head=data.word(descriptor);
            path.kind=static_cast<int>(head>>24);
            path.count=static_cast<int>(head&65535u);
            require((head&0x00ff0000u)==0 && (path.kind==2 || path.kind==3) && path.count>=2 && path.count<=32767);
            path.points.reserve(static_cast<std::size_t>(path.count)+2);
            path.times.reserve(static_cast<std::size_t>(path.count));
            path.speeds.reserve(static_cast<std::size_t>(path.count)-1);
            path.tension=data.scalar(descriptor+4);
            path.duration=data.scalar(descriptor+12);
            require(path.duration>0);
            const auto points=data.word(descriptor+8), times=data.word(descriptor+16), speeds=data.word(descriptor+20);
            for (int i=0;i<path.count+2;++i) {
                const auto point=points+static_cast<std::uint32_t>(i)*12;
                path.points.push_back({data.scalar(point),data.scalar(point+4),data.scalar(point+8)});
            }
            for (int i=0;i<path.count;++i) {
                const auto time=data.scalar(times+static_cast<std::uint32_t>(i)*4);
                require(time>=0 && time<=1 && (i==0 || time>path.times.back()));
                path.times.push_back(time);
            }
            require(path.times.front()==0 && path.times.back()==1);
            for (int i=0;i<path.count-1;++i) {
                IsleReferenceSpeed speed{};
                for (unsigned j=0;j<5;++j) speed.c[j]=data.scalar(speeds+static_cast<std::uint32_t>(i)*20+j*4);
                path.speeds.push_back(speed);
            }
        }
    }
}
PathSample CartNumericReference::sample(bool intro,PathDomain domain,float parameter) const {
    require(std::isfinite(parameter));
    const auto channel=static_cast<unsigned>(domain);
    require(channel<3);
    const auto& path=paths_[(intro?3u:0u)+channel];
    // Original pure functions take mutable pointers but do not mutate inputs.
    IsleReferencePath semantic{path.kind,path.count,path.tension,path.duration,
        const_cast<IsleReferencePoint*>(path.points.data()),const_cast<float*>(path.times.data()),
        const_cast<IsleReferenceSpeed*>(path.speeds.data())};
    const auto mapped=isle_reference_map(&semantic,std::clamp(parameter,0.0f,1.0f));
    IsleReferencePoint point{};
    isle_reference_position(&point,&semantic,mapped);
    require(std::isfinite(mapped) && std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
    return {{point.x,point.y,point.z},mapped};
}
}
