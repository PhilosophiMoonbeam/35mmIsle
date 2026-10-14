#include "35mmisle/vpk0.hpp"

#include <array>
#include <cstdint>

namespace isle::vpk0 {
namespace {

class Bits {
public:
    explicit Bits(std::span<const std::byte> input) noexcept : input_(input) {}

    bool read(unsigned count, std::uint32_t& value) noexcept {
        value = 0;
        for (unsigned i = 0; i < count; ++i) {
            if (byte_ == input_.size()) {
                return false;
            }
            const auto current = std::to_integer<unsigned>(input_[byte_]);
            value = (value << 1) | ((current >> (7 - bit_)) & 1U);
            if (++bit_ == 8) {
                bit_ = 0;
                ++byte_;
            }
        }
        return true;
    }

    std::size_t consumed() const noexcept { return byte_ + (bit_ != 0 ? 1 : 0); }

private:
    std::span<const std::byte> input_;
    std::size_t byte_ = 0;
    unsigned bit_ = 0;
};

constexpr std::uint8_t no_node = 64;

struct Node {
    std::uint8_t left = no_node;
    std::uint8_t right = no_node;
    std::uint8_t width = 0;
};

class Trees {
public:
    Error parse(Bits& bits, std::uint8_t& root) noexcept {
        std::array<std::uint8_t, 20> pending{};
        std::size_t size = 0;
        for (;;) {
            std::uint32_t flag = 0;
            if (!bits.read(1, flag)) {
                return Error::truncated;
            }
            if (flag != 0 && size < 2) {
                root = size == 0 ? no_node : pending[0];
                return Error::none;
            }
            if (flag == 0) {
                std::uint32_t width = 0;
                if (!bits.read(8, width)) {
                    return Error::truncated;
                }
                if (width > 32) {
                    return Error::invalid_bit_width;
                }
                if (size == pending.size() || used_ == nodes_.size()) {
                    return Error::invalid_tree;
                }
                nodes_[used_].width = static_cast<std::uint8_t>(width);
                pending[size++] = static_cast<std::uint8_t>(used_++);
            } else {
                if (used_ == nodes_.size()) {
                    return Error::invalid_tree;
                }
                const auto right = pending[--size];
                const auto left = pending[--size];
                nodes_[used_].left = left;
                nodes_[used_].right = right;
                pending[size++] = static_cast<std::uint8_t>(used_++);
            }
        }
    }

    Error sample(Bits& bits, std::uint8_t root, std::uint32_t& value) const noexcept {
        if (root == no_node) {
            return Error::invalid_tree;
        }
        auto current = root;
        while (nodes_[current].left != no_node) {
            std::uint32_t branch = 0;
            if (!bits.read(1, branch)) {
                return Error::truncated;
            }
            current = branch == 0 ? nodes_[current].left : nodes_[current].right;
        }
        return bits.read(nodes_[current].width, value) ? Error::none : Error::truncated;
    }

private:
    std::array<Node, 64> nodes_{};
    std::size_t used_ = 0;
};

} // namespace

Result decode(std::span<const std::byte> input, std::span<std::byte> output) noexcept {
    if (input.size() < 4) {
        return {Error::truncated, input.size(), 0};
    }
    constexpr std::array<std::byte, 4> magic{
        std::byte{'v'}, std::byte{'p'}, std::byte{'k'}, std::byte{'0'}};
    for (std::size_t i = 0; i < magic.size(); ++i) {
        if (input[i] != magic[i]) {
            return {Error::bad_magic, magic.size(), 0};
        }
    }
    if (input.size() < 9) {
        return {Error::truncated, input.size(), 0};
    }
    std::uint32_t declared = 0;
    for (std::size_t i = 4; i < 8; ++i) {
        declared = (declared << 8) | std::to_integer<std::uint32_t>(input[i]);
    }
    if (declared > output.size()) {
        return {Error::output_too_small, 9, 0};
    }
    const bool sampled = input[8] != std::byte{0};
    Bits bits(input.subspan(9));
    Trees trees;
    std::uint8_t offset_root = no_node;
    std::uint8_t length_root = no_node;
    std::size_t written = 0;
    const auto result = [&](Error error) noexcept {
        return Result{error, 9 + bits.consumed(), written};
    };
    auto error = trees.parse(bits, offset_root);
    if (error != Error::none) {
        return result(error);
    }
    error = trees.parse(bits, length_root);
    if (error != Error::none) {
        return result(error);
    }
    while (written < declared) {
        std::uint32_t flag = 0;
        if (!bits.read(1, flag)) {
            return result(Error::truncated);
        }
        if (flag == 0) {
            std::uint32_t literal = 0;
            if (!bits.read(8, literal)) {
                return result(Error::truncated);
            }
            output[written++] = static_cast<std::byte>(literal);
            continue;
        }
        std::uint32_t offset = 0;
        error = trees.sample(bits, offset_root, offset);
        if (error != Error::none) {
            return result(error);
        }
        std::uint64_t distance = offset;
        if (sampled) {
            std::uint32_t residual = 0;
            if (offset <= 2) {
                residual = offset + 1;
                error = trees.sample(bits, offset_root, offset);
                if (error != Error::none) {
                    return result(error);
                }
            }
            distance = std::uint64_t{4} * offset + residual;
        }
        std::uint32_t length = 0;
        error = trees.sample(bits, length_root, length);
        if (error != Error::none) {
            return result(error);
        }
        // A zero-length token consumes input but never addresses output storage.
        if (length == 0) {
            continue;
        }
        if (sampled) {
            if (distance < 8) {
                return result(Error::invalid_backreference);
            }
            distance -= 8;
        }
        if (distance == 0 || distance > written || length > declared - written) {
            return result(Error::invalid_backreference);
        }
        const auto back = static_cast<std::size_t>(distance);
        for (std::uint32_t i = 0; i < length; ++i) {
            output[written] = output[written - back];
            ++written;
        }
    }
    return result(Error::none);
}

} // namespace isle::vpk0
