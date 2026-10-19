#ifndef ISLE_SOURCE_VERTEX_PUBLICATION_HPP
#define ISLE_SOURCE_VERTEX_PUBLICATION_HPP
#include "egg_source.h"
#include "actor_resources.hpp"
#include "graphics_task.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace isle::source_geometry {
// Entry-owned, fixed before Game construction; retained through CPU/GPU/Photo joins.
// Only Original C writes vertices/revision, under the Source-tick quiescence fence.
struct VertexPublication {
    std::uint32_t guest_address;
    std::span<const IsleEggVertex> vertices;
    const std::uint64_t* revision;
};
class Publisher final {
    struct Credit { std::uint64_t revision = 0; bool resident = false; };
    std::span<const VertexPublication> views_;
    std::vector<Credit> credits_;
    const std::uint32_t* ram_ = nullptr;
    std::uint64_t epoch_ = 0;
    static void require(bool ok) { if (!ok) throw std::invalid_argument("Source vertex publication violates admitted Actor geometry/ownership"); }
    static bool overlap(std::uintptr_t a, std::size_t n, std::uintptr_t b, std::size_t m) {
        require(a <= UINTPTR_MAX-n && b <= UINTPTR_MAX-m);
        return n && m && a < b+m && b < a+n;
    }
public:
    Publisher(std::span<const VertexPublication> views, const actor::Resources& resources,
              std::span<const menu::RamRange> complete_reservations) : views_(views), credits_(views.size()) {
        static_assert(sizeof(IsleEggVertex) == 16);
        static_assert(offsetof(IsleEggVertex,unk_06) == 6 && offsetof(IsleEggVertex,unk_0C) == 12 && offsetof(IsleEggVertex,unk_0F) == 15);
        resources.require_live();
        for (std::size_t i=0;i<views.size();++i) {
            const auto& v=views[i];
            require(v.revision && v.vertices.data() && !v.vertices.empty());
            require(v.vertices.size() <= menu::rdram_size/16 && v.guest_address >= 0x80000000u);
            const auto start=std::size_t(v.guest_address-0x80000000u), bytes=v.vertices.size_bytes();
            require(start%8 == 0 && start <= menu::rdram_size && bytes <= menu::rdram_size-start);
            require(reinterpret_cast<std::uintptr_t>(v.vertices.data())%alignof(IsleEggVertex)==0);
            require(reinterpret_cast<std::uintptr_t>(v.revision)%alignof(std::uint64_t)==0);
            for (const auto r:complete_reservations) require(!overlap(start,bytes,r.offset,r.size));
            for (std::size_t j=0;j<i;++j) {
                const auto& prior=views[j];
                require(!overlap(start,bytes,prior.guest_address-0x80000000u,prior.vertices.size_bytes()));
                require(!overlap(reinterpret_cast<std::uintptr_t>(v.vertices.data()),bytes,
                    reinterpret_cast<std::uintptr_t>(prior.vertices.data()),prior.vertices.size_bytes()));
            }
            // Every destination byte must be admitted by an actual selected gSPVertex
            // load, itself backed by the exact immutable installation span provenance.
            std::size_t cursor=start;
            while(cursor<start+bytes) {
                auto next=cursor;
                for(const auto& load:resources.vertex_loads()) {
                    const auto& wire=load.wire;
                    require(wire.guest_address>=0x80000000u && wire.bytes.size()==std::size_t(load.count)*16);
                    bool provenance=false;
                    for(const auto& span:resources.installation_spans()) {
                        if(wire.guest_address<span.guest_address) continue;
                        const auto delta=std::size_t(wire.guest_address-span.guest_address);
                        if(delta<=span.bytes.size() && wire.bytes.size()<=span.bytes.size()-delta &&
                           wire.bytes.data()==span.bytes.data()+delta) provenance=true;
                    }
                    require(provenance);
                    const auto at=std::size_t(wire.guest_address-0x80000000u);
                    require(at<=menu::rdram_size && wire.bytes.size()<=menu::rdram_size-at);
                    if(at<=cursor && cursor<at+wire.bytes.size()) {
                        require((start>=at ? start-at : at-start)%16==0);
                        if(at+wire.bytes.size()>next) next=at+wire.bytes.size();
                    }
                }
                require(next>cursor); cursor=next;
            }
        }
    }
    void validate(std::span<std::uint32_t> words, std::uint64_t epoch) const {
        require(words.data() && words.size_bytes()==menu::rdram_size && epoch);
        const auto base=reinterpret_cast<std::uintptr_t>(words.data());
        auto disjoint=[&](const void* p,std::size_t bytes) {
            require((!bytes || p) && !overlap(base,words.size_bytes(),reinterpret_cast<std::uintptr_t>(p),bytes));
        };
        disjoint(this,sizeof(*this)); disjoint(views_.data(),views_.size_bytes());
        disjoint(credits_.data(),credits_.size()*sizeof(Credit));
        for(std::size_t i=0;i<views_.size();++i) {
            const auto& v=views_[i];
            require(!credits_[i].resident || *v.revision>=credits_[i].revision);
            disjoint(v.vertices.data(),v.vertices.size_bytes()); disjoint(v.revision,sizeof(*v.revision));
            require(!overlap(reinterpret_cast<std::uintptr_t>(v.vertices.data()),v.vertices.size_bytes(),
                             reinterpret_cast<std::uintptr_t>(v.revision),sizeof(*v.revision)));
        }
    }
    // Called only after all fallible producer staging, inside Frame PreparationLease,
    // after immutable installation. No allocation, native overlay, or byte mirror.
    void publish(std::span<std::uint32_t> words,std::uint64_t epoch) noexcept {
        graphics::Memory memory(words);
        const bool fresh=ram_!=words.data() || epoch_!=epoch;
        for(std::size_t i=0;i<views_.size();++i) {
            const auto& v=views_[i]; auto& credit=credits_[i];
            if(!fresh && credit.resident && credit.revision==*v.revision) continue;
            auto at=v.guest_address-0x80000000u;
            for(const auto& vertex:v.vertices) {
                const std::uint16_t xyz[]{static_cast<std::uint16_t>(vertex.unk_00),static_cast<std::uint16_t>(vertex.unk_02),static_cast<std::uint16_t>(vertex.unk_04)};
                for(unsigned axis=0;axis<3;++axis) {
                    memory.byte(at+axis*2,static_cast<std::uint8_t>(xyz[axis]>>8));
                    memory.byte(at+axis*2+1,static_cast<std::uint8_t>(xyz[axis]));
                }
                for(unsigned b=0;b<6;++b) memory.byte(at+6+b,vertex.unk_06[b]);
                memory.byte(at+12,static_cast<std::uint8_t>(vertex.unk_0C));
                memory.byte(at+13,static_cast<std::uint8_t>(vertex.unk_0D));
                memory.byte(at+14,static_cast<std::uint8_t>(vertex.unk_0E));
                memory.byte(at+15,vertex.unk_0F); at+=16;
            }
            credit.revision=*v.revision; credit.resident=true;
        }
        ram_=words.data(); epoch_=epoch;
    }
};
}
#endif
