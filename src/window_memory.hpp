#ifndef ISLE_WINDOW_MEMORY_HPP
#define ISLE_WINDOW_MEMORY_HPP
#include <cstddef>
#include <memory>
#include <span>

namespace isle::window {
// First-fit UIMem free-list policy, with native pointer/size/alignment storage.
// Borrowed backing must outlive this heap and every outstanding allocation.
class Heap final {
public:
    explicit Heap(std::size_t bytes);
    explicit Heap(std::span<std::byte> backing);
    ~Heap();
    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;
    void* allocate(std::size_t bytes) noexcept;
    void deallocate(void* data) noexcept;
    // Resolve only actual allocated payload starts by walking this heap's
    // chunk chain. Foreign/interior/already-free pointers are never dereferenced.
    bool deallocate_checked(void* data) noexcept;
    bool owns_allocation(const void* data, std::size_t bytes = 0) const noexcept;
    void* reallocate(void* data, std::size_t bytes) noexcept;
    std::size_t allocated() const noexcept;
    std::size_t capacity() const noexcept { return size_; }
    static std::size_t overhead() noexcept;
    static std::size_t minimum_chunk() noexcept;
private:
    struct Chunk {
        Chunk* previous;
        std::size_t size;
        bool allocated;
        union { struct { Chunk* next; Chunk* previous; } link; std::byte data[1]; } payload;
    };
    void initialize(std::span<std::byte> backing);
    void append(Chunk*) noexcept;
    void unlink(Chunk*) noexcept;
    Chunk* split(Chunk*, std::size_t) noexcept;
    void merge(Chunk*) noexcept;
    Chunk* next(Chunk*) const noexcept;
    Chunk* find_allocation(const void*) const noexcept;
    std::unique_ptr<std::max_align_t[]> owned_;
    std::byte* base_ = nullptr;
    std::size_t size_ = 0;
    Chunk sentinel_{};
};
} // namespace isle::window
#endif
