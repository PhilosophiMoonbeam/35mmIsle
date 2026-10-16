#include "35mmisle/focus_detector.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
namespace f=isle::focus;
int failures=0;
void check(bool value,const char* text) {
    if (!value) { ++failures; std::cerr<<"FAIL: "<<text<<'\n'; }
}
struct Fixture {
    static constexpr std::uint32_t base=64;
    std::vector<std::uint32_t> words=std::vector<std::uint32_t>(base/4+20*32+16,0xa5a5a5a5);
    void pixel(unsigned tile,unsigned pixel,std::uint16_t value) {
        const auto address=base+tile*128+pixel*2;
        auto* bytes=reinterpret_cast<unsigned char*>(words.data());
        bytes[address^3u]=static_cast<unsigned char>(value>>8);
        bytes[(address+1)^3u]=static_cast<unsigned char>(value);
    }
    void fill(unsigned tile,std::uint16_t value) {
        for (unsigned p=0;p<64;++p) pixel(tile,p,value);
    }
    void changed(unsigned tile,unsigned count,std::uint16_t from=0x1356) {
        fill(tile,from);
        for (unsigned p=0;p<count;++p) pixel(tile,p,static_cast<std::uint16_t>(from^1u));
    }
};
f::Photo photo(f::Object first,f::Object second=0,f::Object previous=0) {
    f::Photo result; result.objects[0]=first; result.objects[1]=second; result.focus=previous; return result;
}
bool equal(const f::Focus& a,const f::Focus& b) {
    return a.object==b.object && a.pokemon_id==b.pokemon_id && a.flags==b.flags && a.present==b.present && a.context==b.context;
}
void threshold_and_full_color() {
    Fixture memory; f::Detector detector;
    const std::array rows{f::Subject{0,-1,0},f::Subject{11,54,4}};
    memory.fill(0,0x1356); memory.changed(1,31);
    check(detector.stage(0,rows,photo(11))==f::Error::none && detector.analyze(0,memory.words,Fixture::base)==f::Error::none,
          "valid source history rejected");
    check(!detector.computed().present && detector.computed().object==0 && detector.photo(0)->focus==0,
          "31 changed pixels incorrectly qualify");
    memory.changed(1,32);
    check(detector.analyze(0,memory.words,Fixture::base)==f::Error::none && detector.computed().present &&
          detector.computed().object==11 && detector.computed().pokemon_id==54 && detector.computed().flags==4 &&
          detector.photo(0)->focus==11,"32 full16bit changes, including coverage-only differences, must qualify");
    detector.publish(); const auto published=detector.published();
    memory.fill(1,0x1356);
    check(detector.analyze(0,memory.words,Fixture::base)==f::Error::none && !detector.computed().present &&
          detector.computed().object==0 && detector.computed().pokemon_id==54 && detector.computed().flags==4 &&
          detector.photo(0)->focus==0,"no-hit analysis must clear identity/photo focus but retain source ID/flags history");
    check(equal(detector.published(),published),"analysis published before the source CopyInfo boundary");
    detector.publish();
    check(equal(detector.published(),detector.computed()),"CopyInfo lost the analyzed source context/result");
}
void last_candidate_and_photo_membership() {
    Fixture memory; f::Detector detector;
    const std::array rows{f::Subject{0,-1,0},f::Subject{11,16,1},f::Subject{22,48,4}};
    memory.fill(0,0); memory.fill(1,0xffff); memory.fill(2,0);
    check(detector.stage(1,rows,photo(11,22))==f::Error::none && detector.analyze(1,memory.words,Fixture::base)==f::Error::none &&
          detector.computed().present && detector.computed().object==22 && detector.computed().context==1,
          "last qualifying source draw must win instead of largest/first/nearest subject");
    check(detector.stage(1,rows,photo(11,0,99))==f::Error::none && detector.analyze(1,memory.words,Fixture::base)==f::Error::none &&
          !detector.computed().present && detector.computed().object==0 && detector.computed().pokemon_id==48 &&
          detector.computed().flags==4 && detector.photo(1)->focus==99,
          "last nonmember must invalidate focus without earlier fallback or clearing captured photo focus");
    for (const std::int32_t id:{1004,1018,1022,1035}) {
        const std::array sign_rows{f::Subject{0,-1,0},f::Subject{33,id,0}};
        check(detector.stage(0,sign_rows,{})==f::Error::none && detector.analyze(0,memory.words,Fixture::base)==f::Error::none &&
              detector.computed().present && detector.computed().object==33 && detector.photo(0)->focus==33,
              "source sign exception wrongly requires ordinary photo membership");
    }
    const std::array ordinary{f::Subject{0,-1,0},f::Subject{33,1003,0}};
    check(detector.stage(0,ordinary,{})==f::Error::none && detector.analyze(0,memory.words,Fixture::base)==f::Error::none &&
          !detector.computed().present,"ordinary system actor incorrectly received the four-sign exception");
}
void snapshot_and_two_context_cleanup() {
    Fixture memory; f::Detector detector;
    std::array rows{f::Subject{0,-1,0},f::Subject{44,16,4}};
    auto membership=photo(44);
    memory.fill(0,0); memory.fill(1,0xffff);
    check(detector.stage(0,rows,membership)==f::Error::none && detector.stage(1,rows,membership)==f::Error::none,
          "two source capture contexts rejected");
    rows[1]={55,54,0}; membership.objects[0]=55;
    check(detector.analyze(0,memory.words,Fixture::base)==f::Error::none && detector.computed().present &&
          detector.computed().object==44 && detector.computed().pokemon_id==16 && detector.computed().flags==4,
          "capture metadata/photo membership was borrowed instead of snapshotted at drawing");
    detector.publish(); const auto published=detector.published();
    detector.cleanup(44);
    check(!detector.computed().present && detector.computed().object==44 && equal(detector.published(),published),
          "source cleanup must clear computed flag only and defer published-info change");
    check(detector.analyze(1,memory.words,Fixture::base)==f::Error::none && !detector.computed().present &&
          detector.computed().object==0,"cleanup missed pending second-context subject identity");
    check(detector.analyze(0,memory.words,Fixture::base)==f::Error::none && !detector.computed().present,
          "cleanup missed first-context subject identity");
    check(detector.stage(0,rows,membership)==f::Error::none && detector.analyze(0,memory.words,Fixture::base)==f::Error::none &&
          detector.computed().present && detector.computed().object==55,
          "fresh native identity could not use a context after retired history removal");
}
void late_rejection_and_capacity() {
    Fixture memory; f::Detector detector;
    const std::array rows{f::Subject{0,-1,0},f::Subject{77,54,4}};
    memory.fill(0,0); memory.fill(1,0xffff);
    check(detector.stage(1,rows,photo(77))==f::Error::none && detector.analyze(1,memory.words,Fixture::base)==f::Error::none,
          "atomicity subject setup rejected");
    detector.publish(); const auto before=detector.computed(),published=detector.published();
    const auto photo_before=detector.photo(1)->focus;
    check(detector.analyze(1,std::span(memory.words).first(Fixture::base/4+63),Fixture::base)==f::Error::extent &&
          detector.analyze(1,memory.words,Fixture::base+4)==f::Error::extent &&
          detector.analyze(2,memory.words,Fixture::base)==f::Error::invalid_context && equal(detector.computed(),before) &&
          equal(detector.published(),published) && detector.photo(1)->focus==photo_before,
          "late extent/alignment/context rejection partially changed prior focus/photo publication");
    const std::array<f::Subject,21> overflow{};
    check(detector.stage(1,overflow,{})==f::Error::extent && detector.stage(2,rows,{})==f::Error::invalid_context &&
          detector.analyze(1,memory.words,Fixture::base)==f::Error::none && detector.computed().object==77,
          "rejected metadata replaced the previously admitted capture");
    std::array<f::Subject,20> full{}; full[0]={0,-1,0}; full[19]={88,48,0};
    for (unsigned i=0;i<19;++i) memory.fill(i,0);
    memory.fill(19,0xffff);
    check(detector.stage(0,full,photo(88))==f::Error::none && detector.analyze(0,memory.words,Fixture::base)==f::Error::none &&
          detector.computed().present && detector.computed().object==88,
          "last valid source history row was excluded by an off-by-one capacity guard");
    check(detector.stage(0,{},photo(88,0,99))==f::Error::none && detector.analyze(0,{},0xffffffffu)==f::Error::none &&
          !detector.computed().present && detector.photo(0)->focus==0,
          "empty source history accessed unused color storage or retained photo focus");
}
std::uint16_t read_half(std::span<const std::uint32_t> words,std::size_t address) {
    const auto* bytes=reinterpret_cast<const unsigned char*>(words.data());
    return static_cast<std::uint16_t>((std::uint16_t(bytes[address^3u])<<8)|bytes[(address+1)^3u]);
}
void source_center_mark() {
    Fixture history; f::Detector detector;
    const std::array rows{f::Subject{0,-1,0},f::Subject{99,54,4}};
    history.fill(0,0); history.fill(1,0xffff);
    check(detector.stage(0,rows,photo(99))==f::Error::none && detector.analyze(0,history.words,Fixture::base)==f::Error::none,
          "center-mark subject setup rejected");
    constexpr std::uint32_t framebuffer=64;
    std::vector<std::uint32_t> words(framebuffer/4+320*240/2+16,0x12345678);
    const auto original=words;
    check(detector.mark_center(words,framebuffer,0)==f::Error::none && words==original,
          "nonnegative direction marked the framebuffer");
    check(detector.mark_center(std::span(words).first(words.size()-32),framebuffer,-1)==f::Error::extent && words==original,
          "short framebuffer rejection partially painted the center");
    check(detector.mark_center(words,framebuffer,-1)==f::Error::none,"source center mark rejected valid canonical framebuffer");
    unsigned changed=0;
    for (unsigned y=0;y<240;++y) for (unsigned x=0;x<320;++x) {
        const auto address=framebuffer+(y*320+x)*2;
        const bool mark=y>=117 && y<=121 && x>=157 && x<=161 &&
            !((y==117 || y==121) && (x==157 || x==161));
        const auto value=read_half(words,address),before=read_half(original,address);
        if (value!=before) ++changed;
        check(value==(mark ? 0xf801 : before),"center mark changed a corner, neighbor, halfword or wrong source coordinate");
    }
    check(changed==21 && std::equal(words.begin(),words.begin()+framebuffer/4,original.begin()) &&
          std::equal(words.end()-16,words.end(),original.end()-16),"source red mark/outer framebuffer guard mismatch");
    const std::array unflagged{f::Subject{0,-1,0},f::Subject{99,54,2}};
    check(detector.stage(0,unflagged,photo(99))==f::Error::none && detector.analyze(0,history.words,Fixture::base)==f::Error::none &&
          detector.mark_center({},0xffffffffu,-1)==f::Error::none,
          "unused framebuffer was accessed without source focused flag4");
    detector.cleanup(99);
    check(detector.mark_center({},0xffffffffu,-1)==f::Error::none,"retired source focus accessed framebuffer");
}
} // namespace
int main() {
    threshold_and_full_color(); last_candidate_and_photo_membership(); snapshot_and_two_context_cleanup();
    late_rejection_and_capacity(); source_center_mark();
    if (failures) return 1;
    std::cout<<"Source live focus color-history, cleanup/publication and canonical center-mark contracts passed\n";
}
