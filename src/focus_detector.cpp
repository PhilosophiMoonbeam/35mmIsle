#include "35mmisle/focus_detector.hpp"

#include <algorithm>
#include <limits>

namespace isle::focus {
namespace {
bool sign(std::int32_t id) noexcept {
    return id==1004 || id==1018 || id==1022 || id==1035;
}
bool changed_half(std::uint32_t value) noexcept { return value!=0; }
}
Error Detector::stage(std::uint16_t context,std::span<const Subject> subjects,const Photo& photo) noexcept {
    if (context>=contexts_.size()) return Error::invalid_context;
    if (subjects.size()>history_capacity) return Error::extent;
    auto& destination=contexts_[context];
    if (destination.retained || destination.generation==std::numeric_limits<std::uint64_t>::max())
        return Error::invalid_context;
    destination.analyzed=false;
    std::copy(subjects.begin(),subjects.end(),destination.subjects.begin());
    destination.count=subjects.size();
    destination.photo=photo;
    ++destination.generation;
    return Error::none;
}
Error Detector::append(std::uint16_t context,std::span<const Subject> subjects) noexcept {
    if (context>=contexts_.size()) return Error::invalid_context;
    auto& destination=contexts_[context];
    if (destination.retained) return Error::invalid_context;
    if (subjects.size()>history_capacity-destination.count) return Error::extent;
    std::copy(subjects.begin(),subjects.end(),destination.subjects.begin()+static_cast<std::ptrdiff_t>(destination.count));
    destination.count+=subjects.size();
    return Error::none;
}
std::span<const Subject> Detector::history(std::uint16_t context) const noexcept {
    if (context>=contexts_.size()) return {};
    const auto& source=contexts_[context];
    return std::span(source.subjects).first(source.count);
}
Error Detector::analyze(std::uint16_t context,std::span<const std::uint32_t> words,
                        std::uint32_t regions_offset) noexcept {
    if (context>=contexts_.size()) return Error::invalid_context;
    auto& source=contexts_[context];
    constexpr std::size_t region_words=tile_bytes/sizeof(std::uint32_t);
    const auto start=static_cast<std::size_t>(regions_offset)/sizeof(std::uint32_t);
    if (source.count>1 && ((regions_offset&63u) || start>words.size() ||
        source.count*region_words>words.size()-start)) return Error::extent;
    auto result=computed_;
    result.context=context;
    result.present=false;
    for (std::size_t i=1;i<source.count;++i) if (source.subjects[i].object) {
        unsigned changed=0;
        const auto before=start+(i-1)*region_words,after=start+i*region_words;
        for (std::size_t j=0;j<region_words && changed<32;++j) {
            const auto difference=words[before+j]^words[after+j];
            changed+=static_cast<unsigned>(changed_half(difference>>16));
            changed+=static_cast<unsigned>(changed_half(difference&0xffffu));
        }
        if (changed>=32) {
            static_cast<Subject&>(result)=source.subjects[i];
            result.present=true;
        }
    }
    auto photo_focus=source.photo.focus;
    if (result.present && result.object) {
        if (sign(result.pokemon_id) ||
            std::find(source.photo.objects.begin(),source.photo.objects.end(),result.object)!=source.photo.objects.end())
            photo_focus=result.object;
        else {
            result.present=false;
            result.object=0;
            // Source membership failure does not clear the captured photo focus
            // or try the earlier qualifying subject. ID/flags also remain last.
        }
    } else {
        photo_focus=0;
        result.object=0;
        result.present=false;
    }
    source.photo.focus=photo_focus;
    computed_=result;
    source.completed=result;
    source.analyzed=true;
    return Error::none;
}
const Photo* Detector::photo(std::uint16_t context) const noexcept {
    return context<contexts_.size() ? &contexts_[context].photo : nullptr;
}
void Detector::publish() noexcept { published_=computed_; }
void Detector::clear_published_presence() noexcept { published_.present=false; }
bool Detector::context_available(std::uint16_t context) const noexcept {
    return context<contexts_.size() && !contexts_[context].retained;
}
std::uint64_t Detector::generation(std::uint16_t context) const noexcept {
    return context<contexts_.size()?contexts_[context].generation:0;
}
Error Detector::retain_context(std::uint16_t context,std::uint64_t generation) noexcept {
    if(context>=contexts_.size() || !generation) return Error::invalid_context;
    auto& value=contexts_[context];
    if(value.retained || generation!=value.generation) return Error::invalid_context;
    value.retained=true;
    return Error::none;
}
Error Detector::release_context(std::uint16_t context,std::uint64_t generation) noexcept {
    if(context>=contexts_.size()) return Error::invalid_context;
    auto& value=contexts_[context];
    if(!value.retained || value.generation!=generation) return Error::invalid_context;
    value.retained=false;
    return Error::none;
}
Error Detector::publish_context(std::uint16_t context,std::uint64_t generation) noexcept {
    if(context>=contexts_.size()) return Error::invalid_context;
    const auto& value=contexts_[context];
    if(!value.retained || !value.analyzed || value.generation!=generation) return Error::invalid_context;
    published_=value.completed;
    return Error::none;
}
void Detector::cleanup(Object object) noexcept {
    if (!object) return;
    for (auto& context:contexts_) for (auto& subject:context.subjects)
        if (subject.object==object) subject.object=0;
    if (computed_.present && computed_.object==object) computed_.present=false;
    for (auto& context:contexts_)
        if(context.completed.present && context.completed.object==object) context.completed.present=false;
}
Error Detector::mark_center(std::span<std::uint32_t> words,std::uint32_t framebuffer,
                            std::int32_t direction) const noexcept {
    if (!computed_.present || direction!=-1 || !(computed_.flags&4u)) return Error::none;
    constexpr std::size_t framebuffer_bytes=320*240*2;
    const auto bytes=words.size_bytes();
    if ((framebuffer&1u) || framebuffer>bytes || framebuffer_bytes>bytes-framebuffer) return Error::extent;
    for (unsigned i=0;i<5;++i) for (unsigned j=0;j<5;++j) if ((i!=0 && i!=4) || (j!=0 && j!=4)) {
        const auto address=static_cast<std::size_t>(framebuffer)+((117+i)*320+(157+j))*2;
        const auto shift=(address&2u) ? 0u : 16u;
        auto& word=words[address/4];
        word=(word&~(0xffffu<<shift))|(0xf801u<<shift);
    }
    return Error::none;
}
} // namespace isle::focus
