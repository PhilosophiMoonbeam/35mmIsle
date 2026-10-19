#include "window_memory.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace isle::window {
std::size_t Heap::overhead() noexcept { return offsetof(Chunk, payload); }
std::size_t Heap::minimum_chunk() noexcept { return sizeof(Chunk); }
Heap::Heap(std::size_t bytes) {
    if (bytes > std::numeric_limits<std::size_t>::max() - sizeof(std::max_align_t))
        throw std::length_error("UI heap extent");
    owned_ = std::make_unique<std::max_align_t[]>((bytes + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    initialize({reinterpret_cast<std::byte*>(owned_.get()), bytes});
}
Heap::Heap(std::span<std::byte> backing) { initialize(backing); }
Heap::~Heap() = default;
void Heap::initialize(std::span<std::byte> backing) {
    if (backing.size() < sizeof(Chunk) || reinterpret_cast<std::uintptr_t>(backing.data()) % alignof(Chunk) ||
        backing.size() % alignof(Chunk)) throw std::invalid_argument("UI heap alignment/extent");
    base_ = backing.data(); size_ = backing.size();
    auto* first = ::new (base_) Chunk{};
    first->size = size_;
    sentinel_.payload.link.next = sentinel_.payload.link.previous = &sentinel_;
    append(first);
}
Heap::Chunk* Heap::next(Chunk* c) const noexcept {
    auto* p = reinterpret_cast<std::byte*>(c) + c->size;
    return p < base_ + size_ ? reinterpret_cast<Chunk*>(p) : nullptr;
}
void Heap::append(Chunk* c) noexcept {
    auto* last = sentinel_.payload.link.previous;
    last->payload.link.next = c; c->payload.link.previous = last;
    c->payload.link.next = &sentinel_; sentinel_.payload.link.previous = c;
}
void Heap::unlink(Chunk* c) noexcept {
    c->payload.link.previous->payload.link.next = c->payload.link.next;
    c->payload.link.next->payload.link.previous = c->payload.link.previous;
}
Heap::Chunk* Heap::split(Chunk* c, std::size_t extent) noexcept {
    auto* child = ::new (reinterpret_cast<std::byte*>(c) + extent) Chunk{};
    child->previous = c; child->size = c->size - extent;
    c->size = extent;
    if (auto* after = next(child)) after->previous = child;
    return child;
}
void Heap::merge(Chunk* c) noexcept {
    auto* after = next(c);
    if (c->previous && !c->previous->allocated) {
        auto* previous = c->previous;
        previous->size += c->size;
        if (after) after->previous = previous;
        c = previous;
    } else { c->allocated = false; append(c); }
    if (after && !after->allocated) {
        if (auto* end = next(after)) end->previous = c;
        c->size += after->size; unlink(after);
    }
}
void* Heap::allocate(std::size_t bytes) noexcept {
    bytes = std::max(bytes, sizeof(Chunk::payload));
    constexpr auto alignment = alignof(Chunk);
    if (bytes > std::numeric_limits<std::size_t>::max() - overhead() - alignment + 1) return nullptr;
    const auto extent = (bytes + overhead() + alignment - 1) & ~(alignment - 1);
    for (auto* c = sentinel_.payload.link.next; c != &sentinel_; c = c->payload.link.next) {
        if (c->size < extent) continue;
        if (c->size - extent > sizeof(Chunk)) append(split(c, extent));
        unlink(c); c->allocated = true; return c->payload.data;
    }
    return nullptr;
}
void Heap::deallocate(void* data) noexcept {
    if (data) merge(reinterpret_cast<Chunk*>(static_cast<std::byte*>(data) - overhead()));
}
Heap::Chunk* Heap::find_allocation(const void* data) const noexcept {
    if (!data) return nullptr;
    const auto requested=reinterpret_cast<std::uintptr_t>(data);
    const auto base=reinterpret_cast<std::uintptr_t>(base_);
    if (requested<base || requested-base>=size_) return nullptr;
    std::size_t offset=0;
    Chunk* previous=nullptr;
    while (offset<size_) {
        if (size_-offset<sizeof(Chunk)) return nullptr;
        auto* chunk=reinterpret_cast<Chunk*>(base_+offset);
        if (chunk->previous!=previous || chunk->size<sizeof(Chunk) ||
            chunk->size%alignof(Chunk) || chunk->size>size_-offset)
            return nullptr;
        if (reinterpret_cast<std::uintptr_t>(chunk->payload.data)==requested)
            return chunk->allocated ? chunk : nullptr;
        previous=chunk;
        offset+=chunk->size;
    }
    return nullptr;
}
bool Heap::owns_allocation(const void* data,std::size_t bytes) const noexcept {
    const auto* chunk=find_allocation(data);
    return chunk && bytes<=chunk->size-overhead();
}
bool Heap::deallocate_checked(void* data) noexcept {
    auto* chunk=find_allocation(data);
    if (!chunk) return false;
    merge(chunk);
    return true;
}
void* Heap::reallocate(void* data, std::size_t bytes) noexcept {
    if (!data) return allocate(bytes);
    auto* c = reinterpret_cast<Chunk*>(static_cast<std::byte*>(data) - overhead());
    bytes = std::max(bytes, sizeof(Chunk::payload));
    constexpr auto alignment = alignof(Chunk);
    if (bytes > std::numeric_limits<std::size_t>::max() - overhead() - alignment + 1) return nullptr;
    const auto extent = (bytes + overhead() + alignment - 1) & ~(alignment - 1);
    if (c->size < extent) {
        auto* after = next(c);
        if (after && !after->allocated && c->size + after->size >= extent) {
            if (auto* end = next(after)) end->previous = c;
            c->size += after->size; unlink(after);
        } else {
            auto* replacement = allocate(bytes);
            if (!replacement) return nullptr;
            // The GAME loop used the header-inclusive size and overread its
            // allocation. Preserve all native payload bytes, never that UB.
            std::memcpy(replacement, data, c->size - overhead());
            deallocate(data); return replacement;
        }
    }
    if (c->size - extent > sizeof(Chunk)) merge(split(c, extent));
    return data;
}
std::size_t Heap::allocated() const noexcept {
    std::size_t result = 0;
    for (auto* c = reinterpret_cast<Chunk*>(base_); c; c = next(c))
        if (c->allocated) result += c->size;
    return result;
}
} // namespace isle::window
