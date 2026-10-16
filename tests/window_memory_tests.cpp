#include "window_memory.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void coalescing_and_growth(){
    isle::window::Heap heap(4096);
    auto* a=static_cast<std::uint8_t*>(heap.allocate(64));
    auto* b=heap.allocate(64);auto* c=heap.allocate(64);
    check(a&&b&&c&&reinterpret_cast<std::uintptr_t>(a)%alignof(void*)==0,"native pointer alignment");
    std::fill_n(a,64,0x5a);heap.deallocate(b);
    check(heap.reallocate(a,96)==a,"growing consumes adjacent free chunk in place");
    check(std::all_of(a,a+64,[](auto x){return x==0x5a;}),"adjacent growth preserves every old payload byte");
    heap.deallocate(c);heap.deallocate(a);
    check(heap.allocated()==0,"free merges previous and next without allocated accounting residue");
    auto* whole=heap.allocate(heap.capacity()-heap.overhead());
    check(whole==a&&heap.allocated()==heap.capacity(),"both-side coalescing restores one complete allocatable arena");
    heap.deallocate(whole);
}
void relocation_and_failure(){
    isle::window::Heap heap(4096);
    auto* a=static_cast<std::uint8_t*>(heap.allocate(80));auto* obstruction=heap.allocate(80);
    std::fill_n(a,80,0x93);
    auto* moved=static_cast<std::uint8_t*>(heap.reallocate(a,240));
    check(moved&&moved!=a,"nonadjacent realloc relocates through first-fit allocation");
    check(std::all_of(moved,moved+80,[](auto x){return x==0x93;}),"relocation copies bounded old native payload, not header-inclusive source overread");
    const auto before=heap.allocated();
    check(heap.reallocate(moved,std::numeric_limits<std::size_t>::max())==nullptr,"overflowed realloc rejects");
    check(heap.allocated()==before&&moved[0]==0x93&&moved[79]==0x93,"failed growth retains original allocation and bytes");
    check(heap.reallocate(moved,16)==moved,"shrink stays in place");
    heap.deallocate(moved);heap.deallocate(obstruction);
    check(heap.allocated()==0,"shrink split and relocation free all native records");
}
void strict_split_boundary(){
    isle::window::Heap heap(isle::window::Heap::overhead()+16+isle::window::Heap::minimum_chunk());
    auto* p=heap.allocate(16);
    check(p&&heap.allocated()==heap.capacity(),"exactly sizeof(native Chunk) remainder is not split: source strict greater-than policy");
    heap.deallocate(p);
    check(heap.allocated()==0&&heap.allocate(std::numeric_limits<std::size_t>::max())==nullptr,"oversized request does not corrupt free ring");
}
}
int main(){try{coalescing_and_growth();relocation_and_failure();strict_split_boundary();std::cout<<"window memory tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
