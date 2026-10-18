// Opt-in PRIVATE_ROM NEW_OWNED_EMPTY_ROOT. Authored records exercise the real
// renderer; they are not captured gameplay photographs or an Original oracle.
// Canonical CPU-Vulkan only; no Host/input/audio/OM/GTL or Windows parity claim.
#include "photo_replay.hpp"
#include "scene_graphics.hpp"
#include "scene_storage.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <cstdint>
#include <string>
#include <vector>
namespace {
namespace pg=isle::photo;
namespace sg=isle::scene::graphics;
namespace fs=std::filesystem;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void native(IsleGameResult value){require(value==ISLE_GAME_OK,"native Game operation failed");}
struct Harness {
    isle::rom::Snapshot snapshot;
    isle::world::ModuleState module;
    isle::scene::Storage storage;
    std::unique_ptr<sg::Frame> frame;
    IslePlayerPhotoData record{};
    std::uint64_t revision=1;
    std::unique_ptr<pg::Replay> replay;
    explicit Harness(const fs::path& rom):snapshot(isle::rom::read_snapshot(rom)) {
        native(storage.create());
        sg::Config config;config.require_cpu=true;
        config.memory.commands={0x6b0000,0x10000};
        config.memory.workspace={0x6c0000,0x10000};
        config.memory.output={0x6d0000,0x40000};
        auto made=sg::Frame::create(snapshot,config);
        require(bool(made),made.message.data());frame=std::move(made.frame);
        std::vector<isle::menu::RamRange> external;
        for(const auto r:frame->reservations()) {
            // Replay/each packet producer already admits immutable main/BSS.
            // Its external plan names only independent display storage.
            if(r.address==0 && r.size==isle::graphics::bss_end)continue;
            external.push_back({r.address,r.size});
        }
        pg::ReplayConfig rc;auto& m=rc.memory;
        m.world={{0x400000,0x20000},{0x420000,0x10000},{0x430000,0x10000}};
        m.subjects={{0x440000,0x60000},{0x4a0000,0x20000},{0x4c0000,0x20000},{}};
        m.items={{0x4e0000,0x10000},{0x4f0000,0x10000},{0x500000,0x10000},{}};
        m.commands={0x510000,0x20000};m.workspace={0x530000,0x10000};m.output={0x540000,0x40000};
        m.effects={0x580000,0x10000};m.effect_assets={0x630000,0x80000};
        m.feedback={0x7be600,0x27000};m.preview_color={0x7be600,0x20d00};m.preview_depth={0x7df300,0x20d00};
        rc.external_reservations=external;
        replay=std::make_unique<pg::Replay>(storage.game(),snapshot,module,
            pg::ReplayGpuAccess{this,acquire,release},rc);
        std::vector<isle::sprite::RamRange> protected_ranges;
        for(const auto r:replay->reservations())protected_ranges.push_back({static_cast<std::uint32_t>(r.offset),static_cast<std::uint32_t>(r.size)});
        if(!frame->reserve_source_gpu_storage(protected_ranges)) {
            const std::string message=frame->message();
            while(!replay->retire())std::this_thread::yield();
            throw std::runtime_error(message);
        }
    }
    ~Harness() {
        if(replay){replay->cancel();while(!replay->retire())std::this_thread::yield();replay.reset();}
        if(frame && !frame->retire())std::terminate();
        if(storage.game() && storage.destroy()!=ISLE_GAME_OK)std::terminate();
    }
    static pg::ReplayAcquire acquire(void* opaque,pg::ReplayGpuBinding& out) noexcept {
        auto& h=*static_cast<Harness*>(opaque);sg::SourceGpuBinding binding;
        switch(h.frame->acquire_source_gpu(binding)) {
        case sg::SourceGpuAcquire::admitted:out={binding.executor,binding.programs,binding.epoch,binding.admission};return pg::ReplayAcquire::admitted;
        case sg::SourceGpuAcquire::pending:return pg::ReplayAcquire::pending;
        case sg::SourceGpuAcquire::failed:return pg::ReplayAcquire::failed;
        }
        return pg::ReplayAcquire::failed;
    }
    static bool release(void* opaque,const pg::ReplayGpuBinding& binding) noexcept {
        return static_cast<Harness*>(opaque)->frame->release_source_gpu(
            {binding.executor,binding.programs,binding.epoch,binding.admission});
    }
    void prepare(int course,bool subject) {
        record={};record.levelID=static_cast<std::int8_t>(course);
        record.unk_08={0,60,300};record.unk_14={0,60,-250};
        for(auto& p:record.pokemons)p.pokemonID=-1;
        for(auto& i:record.items)i.itemType=-1;
        for(auto& e:record.effects)e.textureID=-1;
        if(subject){auto& p=record.pokemons[0];p.pokemonID=16;p.position={0,30,-250};}
        ++revision;
    }
    pg::ReplayRequest request() const {
        return {pg::ReplayKind::preview,{&record,&revision,revision},false,103,77,1};
    }
    std::vector<std::byte> preview() {
        require(replay->begin(request()),replay->error());
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);
        for(;;) {
            const auto step=replay->poll();
            if(step.signal==pg::ReplaySignal::result)break;
            if(step.signal==pg::ReplaySignal::failed || step.signal==pg::ReplaySignal::cancelled)
                throw std::runtime_error(replay->error());
            require(step.signal==pg::ReplaySignal::pending,"preview invented a Source sound/wait obligation");
            require(std::chrono::steady_clock::now()<deadline,"actual preview GPU deadline expired");
            std::this_thread::yield();
        }
        const auto result=replay->result();
        require(result.kind==pg::ReplayKind::preview && result.width==103 && result.height==77 &&
            result.stride_bytes==640 && result.rgba5551.size()==0x20d00,"canonical completed preview contract changed");
        require(frame->quiescent() && !frame->submission() && !replay->gpu_busy(),
            "result published before real replay GPU/RAM admission closed");
        std::vector<std::byte> copy(result.rgba5551.begin(),result.rgba5551.end());
        require(replay->acknowledge_result(),replay->error());return copy;
    }
};
void ppm(const fs::path& name,std::span<const std::byte> pixels) {
    std::ofstream output(name,std::ios::binary|std::ios::trunc);require(bool(output),"preview output open failed");
    output<<"P6\n104 78\n255\n";
    for(unsigned y=0;y<78;++y)for(unsigned x=0;x<104;++x){
        const auto at=(y*320+x)*2;const auto value=(std::to_integer<unsigned>(pixels[at])<<8)|std::to_integer<unsigned>(pixels[at+1]);
        for(const unsigned shift:{11u,6u,1u}){const auto c=(value>>shift)&31u;output.put(static_cast<char>((c<<3)|(c>>2)));}
    }
    require(bool(output),"preview output write failed");
}
int run(const fs::path& rom,const fs::path& root) {
    require(root.is_absolute() && fs::is_directory(fs::symlink_status(root)) && !fs::is_symlink(fs::symlink_status(root)) && fs::is_empty(root),
        "root must be existing empty absolute nonsymlink owned directory");
    Harness h(rom);
    sg::SourceGpuBinding held;require(h.frame->acquire_source_gpu(held)==sg::SourceGpuAcquire::admitted,"real exclusive admission unavailable");
    auto stale=held;++stale.admission;require(!h.frame->release_source_gpu(stale) && h.frame->busy(),"stale admission released actual ownership");
    require(h.frame->release_source_gpu(held),"real empty-stage admission failed to release");
    for(int course=0;course<7;++course){h.prepare(course,false);const auto image=h.preview();ppm(root/("course-"+std::to_string(course)+".ppm"),image);
        std::cout<<"actual_course_preview="<<course<<" canonical_RGBA5551_bytes="<<image.size()<<'\n';}
    h.prepare(0,true);const auto subject=h.preview();ppm(root/"subject-preview.ppm",subject);
    // Reconstruct the identical camera/course again without the subject. Exact
    // target history and native World ownership persist; only that record differs.
    h.prepare(0,false);const auto terrain=h.preview();ppm(root/"terrain-only.ppm",terrain);
    std::size_t changed=0;for(std::size_t i=0;i<subject.size();++i)changed+=subject[i]!=terrain[i];
    require(changed>0,"real selected subject contributed no canonical pixels");
    h.prepare(0,true);require(h.replay->begin(h.request()),h.replay->error());
    require(h.replay->poll().signal==pg::ReplaySignal::pending && h.replay->gpu_busy() && h.frame->busy() && !h.frame->submission(),
        "genuine replay submission did not retain independent admission/token");
    h.replay->cancel();const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);
    while(!h.replay->retire()){require(std::chrono::steady_clock::now()<deadline,"actual cancellation GPU/native drain timed out");std::this_thread::yield();}
    require(h.frame->quiescent() && !h.replay->gpu_busy() && !h.replay->busy(),"cancelled request retained GPU/native work");
    IsleGameCounts counts{};native(isle_game_counts(h.storage.game(),&counts));
    require(counts.objects==0 && counts.processes==0 && counts.models==0 && counts.cameras==0,"actual replay native owners failed joined retirement");
    std::cout<<"actual_recorded_preview_all7_subject_pixels_and_submitted_cancellation=passed subject_differing_bytes="<<changed
        <<" scope=authored_records_real_GPU_no_Source_score_wait_or_full_game_or_hardware_or_Windows_claim\n";
    return 0;
}
}
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif
    try{require(argc==3,"usage: photo-replay-rsp-smoke PRIVATE_ROM NEW_OWNED_EMPTY_ROOT");return run(fs::path(argv[1]),fs::path(argv[2]));}
    catch(const std::exception& error){std::cerr<<"actual recorded preview consumer: "<<error.what()<<'\n';return 1;}
}
