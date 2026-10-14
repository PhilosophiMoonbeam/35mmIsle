#include "35mmisle/vpk0.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using isle::vpk0::Error;
using isle::vpk0::Result;

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

// Original synthetic fixtures only. No files, extracted assets, or ROM data.
class Fixture {
public:
    explicit Fixture(std::uint32_t length, unsigned method = 0)
        : bytes{std::byte{'v'}, std::byte{'p'}, std::byte{'k'}, std::byte{'0'},
                static_cast<std::byte>(length >> 24), static_cast<std::byte>(length >> 16),
                static_cast<std::byte>(length >> 8), static_cast<std::byte>(length),
                static_cast<std::byte>(method)} {}

    void bits(std::uint32_t value, unsigned width) {
        for (unsigned i = width; i != 0; --i) {
            if (used_ == 0) {
                bytes.push_back(std::byte{0});
            }
            const auto bit = (value >> (i - 1)) & 1U;
            bytes.back() |= static_cast<std::byte>(bit << (7 - used_));
            used_ = (used_ + 1) % 8;
        }
    }

    void leaf(unsigned width) { bits(0, 1); bits(width, 8); }
    void join() { bits(1, 1); }
    void end() { bits(1, 1); }
    void single(unsigned width) { leaf(width); end(); }
    void pair(unsigned left_width, unsigned right_width) {
        leaf(left_width); leaf(right_width); join(); end();
    }
    void literal(unsigned value) { bits(0, 1); bits(value, 8); }
    void copy(unsigned offset, unsigned offset_width, unsigned length, unsigned length_width) {
        bits(1, 1); bits(offset, offset_width); bits(length, length_width);
    }

    std::vector<std::byte> bytes;

private:
    unsigned used_ = 0;
};

std::vector<std::byte> text(std::string_view value) {
    std::vector<std::byte> bytes;
    for (const char character : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(character)));
    }
    return bytes;
}

// Guard both sides of the supplied span and check the unwritten capacity too.
Result expect(std::span<const std::byte> input, Error error,
              std::span<const std::byte> expected, std::size_t capacity,
              std::size_t consumed, const char* description) {
    std::array<std::byte, 258> storage;
    storage.fill(std::byte{0xcc});
    if (capacity > storage.size() - 2 || expected.size() > capacity) {
        check(false, "test fixture fits guard storage");
        return {Error::output_too_small, 0, 0};
    }
    auto destination = std::span<std::byte>(storage).subspan(1, capacity);
    const auto result = isle::vpk0::decode(input, destination);
    check(result.error == error, description);
    check(static_cast<bool>(result) == (error == Error::none), "result boolean matches error");
    check(result.bytes_consumed == consumed, "exact rounded logical consumption");
    check(result.bytes_written == expected.size(), "exact written prefix length");
    check(std::equal(expected.begin(), expected.end(), destination.begin()), "fixed expected output");
    check(storage.front() == std::byte{0xcc}, "output leading guard unchanged");
    check(std::all_of(storage.begin() + static_cast<std::ptrdiff_t>(expected.size() + 1),
                      storage.end(), [](std::byte value) { return value == std::byte{0xcc}; }),
          "output trailing guard and unused capacity unchanged");
    return result;
}

void success(const Fixture& fixture, std::string_view expected, const char* description) {
    const auto bytes = text(expected);
    expect(fixture.bytes, Error::none, bytes, bytes.size() + 3, fixture.bytes.size(), description);
}

void header_and_literals() {
    // Independently fixed wire bytes: empty trees and three literals, 29 bits.
    const std::array<std::byte, 13> fixed{
        std::byte{0x76}, std::byte{0x70}, std::byte{0x6b}, std::byte{0x30},
        std::byte{0}, std::byte{0}, std::byte{0}, std::byte{3}, std::byte{0},
        std::byte{0xc8}, std::byte{0x24}, std::byte{0x22}, std::byte{0x18}};
    const auto abc = text("ABC");
    expect(fixed, Error::none, abc, 3, fixed.size(), "fixed literal wire reference");

    Fixture literals(4);
    literals.end(); literals.end();
    for (unsigned value : {0U, 127U, 128U, 255U}) {
        literals.literal(value);
    }
    const std::array<std::byte, 4> binary{
        std::byte{0}, std::byte{127}, std::byte{128}, std::byte{255}};
    expect(literals.bytes, Error::none, binary, 4, literals.bytes.size(), "binary literals and empty trees");
    const auto logical_size = literals.bytes.size();
    literals.bytes.insert(literals.bytes.end(), {std::byte{0xff}, std::byte{0}, std::byte{0x7b}});
    expect(literals.bytes, Error::none, binary, 9, logical_size, "trailing bytes are not consumed");

    Fixture empty(0);
    empty.end(); empty.end();
    expect(empty.bytes, Error::none, {}, 0, 10, "zero output still parses both trees");

    for (std::size_t size = 0; size < 9; ++size) {
        expect(std::span<const std::byte>(fixed).first(size), Error::truncated, {}, 3, size,
               "every incomplete header length");
    }
    auto bad_magic = fixed;
    bad_magic[0] = std::byte{'V'};
    expect(bad_magic, Error::bad_magic, {}, 3, 4, "magic is case sensitive");
    expect(std::span<const std::byte>(bad_magic).first(4), Error::bad_magic, {}, 3, 4,
           "bad magic takes precedence over incomplete remainder");
    expect(fixed, Error::output_too_small, {}, 2, 9, "capacity rejected before any writes or trees");
    Fixture huge(0xffffffffU);
    expect(huge.bytes, Error::output_too_small, {}, 0, 9, "big endian length uses all four bytes");
    Fixture endian(256);
    expect(endian.bytes, Error::output_too_small, {}, 255, 9, "header length is not native endian");
}

void direct_copies() {
    Fixture overlap(8);
    overlap.single(2); overlap.single(3);
    overlap.literal('A'); overlap.literal('B'); overlap.copy(2, 2, 6, 3);
    success(overlap, "ABABABAB", "forward overlapping copy repeats initialized bytes");

    Fixture branches(6);
    branches.pair(1, 3); branches.pair(0, 3);
    branches.literal('A');
    branches.bits(1, 1); branches.bits(0, 1); branches.bits(1, 1); branches.bits(0, 1);
    branches.literal('B'); branches.literal('C');
    branches.bits(1, 1); branches.bits(1, 1); branches.bits(3, 3);
    branches.bits(1, 1); branches.bits(3, 3);
    success(branches, "ABCABC", "both branches of both trees and zero width length");

    Fixture zero_tokens(2);
    zero_tokens.single(1); zero_tokens.single(0);
    zero_tokens.literal('Q');
    for (unsigned i = 0; i < 100; ++i) {
        zero_tokens.copy(1, 1, 0, 0);
    }
    zero_tokens.literal('R');
    success(zero_tokens, "QR", "zero length tokens advance input without output");

    Fixture wide(4);
    wide.single(32); wide.single(32);
    wide.literal('Z'); wide.copy(1, 32, 3, 32);
    success(wide, "ZZZZ", "32 bit samples avoid oversized shifts");
}

void sampled_copies() {
    for (unsigned method : {1U, 2U, 255U}) {
        for (unsigned residual = 1; residual <= 3; ++residual) {
            Fixture sampled(6, method);
            sampled.single(3); sampled.single(2);
            sampled.literal('A'); sampled.literal('B'); sampled.literal('C');
            sampled.bits(1, 1); sampled.bits(residual - 1, 3); sampled.bits(2, 3);
            sampled.bits(3, 2);
            const auto expected = residual == 1 ? "ABCCCC" : residual == 2 ? "ABCBCB" : "ABCABC";
            success(sampled, expected, "all nonzero methods and residual paths are equivalent");
        }
        Fixture direct(8, method);
        direct.single(3); direct.single(3);
        for (const char value : {'A', 'B', 'C', 'D'}) {
            direct.literal(static_cast<unsigned char>(value));
        }
        direct.copy(3, 3, 4, 3);
        success(direct, "ABCDABCD", "nonzero method direct sample gives four byte distance");
    }
}

void zero_length_tokens() {
    Fixture zero(1);
    zero.single(1); zero.single(0);
    zero.copy(0, 1, 0, 0); zero.literal('A');
    success(zero, "A", "zero-length token requires no previous output");

    Fixture negative(1, 1);
    negative.single(1); negative.single(0);
    negative.bits(1, 1); negative.bits(0, 1); negative.bits(0, 1);
    negative.literal('B');
    success(negative, "B", "zero-length sampled token forms no forward pointer");

    Fixture distant(1);
    distant.single(32); distant.single(0);
    distant.copy(0xffffffffU, 32, 0, 0); distant.literal('C');
    success(distant, "C", "zero-length token forms no out-of-range pointer");
}

void tree_limits() {
    // Left-deep tree: traversal depth is bounded by nodes, not the parser stack.
    Fixture maximum(2);
    maximum.leaf(1);
    for (unsigned i = 1; i < 32; ++i) {
        maximum.leaf(1); maximum.join();
    }
    maximum.end(); maximum.single(1); // 63 + 1 = 64 nodes.
    maximum.literal('M'); maximum.bits(1, 1);
    maximum.bits(0, 31); maximum.bits(1, 1); maximum.bits(1, 1);
    success(maximum, "MM", "all 64 shared nodes and deep iterative traversal");

    Fixture stack_limit(0);
    for (unsigned i = 0; i < 20; ++i) {
        stack_limit.leaf(0);
    }
    for (unsigned i = 1; i < 20; ++i) {
        stack_limit.join();
    }
    stack_limit.end(); stack_limit.end();
    success(stack_limit, "", "20 pending roots are admitted");

    Fixture stack_overflow(0);
    for (unsigned i = 0; i < 21; ++i) {
        stack_overflow.leaf(0);
    }
    expect(stack_overflow.bytes, Error::invalid_tree, {}, 0, stack_overflow.bytes.size(),
           "21st pending root is rejected");

    Fixture pool_overflow(0);
    pool_overflow.leaf(0);
    for (unsigned i = 1; i < 32; ++i) {
        pool_overflow.leaf(0); pool_overflow.join();
    }
    pool_overflow.end(); pool_overflow.leaf(0); pool_overflow.leaf(0);
    expect(pool_overflow.bytes, Error::invalid_tree, {}, 0, pool_overflow.bytes.size(),
           "65th node rejected across both trees");

    for (unsigned width : {33U, 255U}) {
        Fixture invalid(1);
        invalid.leaf(width);
        expect(invalid.bytes, Error::invalid_bit_width, {}, 1, invalid.bytes.size(), "unsupported leaf width");
    }
    Fixture invalid_second(1);
    invalid_second.end(); invalid_second.leaf(33);
    expect(invalid_second.bytes, Error::invalid_bit_width, {}, 1, invalid_second.bytes.size(),
           "length tree leaf width checked too");
}

void backreference_errors() {
    Fixture empty_offset(1);
    empty_offset.end(); empty_offset.single(1); empty_offset.bits(1, 1);
    expect(empty_offset.bytes, Error::invalid_tree, {}, 1, empty_offset.bytes.size(),
           "empty offset tree is rejected only when used");
    Fixture empty_length(2);
    empty_length.single(1); empty_length.end(); empty_length.literal('A');
    empty_length.bits(1, 1); empty_length.bits(1, 1);
    const auto a = text("A");
    expect(empty_length.bytes, Error::invalid_tree, a, 2, empty_length.bytes.size(),
           "empty length tree is rejected only when used");

    for (unsigned distance : {0U, 2U}) {
        Fixture invalid(2);
        invalid.single(2); invalid.single(1);
        invalid.literal('A'); invalid.copy(distance, 2, 1, 1);
        expect(invalid.bytes, Error::invalid_backreference, a, 2, invalid.bytes.size(),
               "zero or beyond-prefix distance rejected");
    }
    Fixture zero_offset(2);
    zero_offset.single(0); zero_offset.single(1);
    zero_offset.literal('A'); zero_offset.copy(0, 0, 1, 1);
    expect(zero_offset.bytes, Error::invalid_backreference, a, 2, zero_offset.bytes.size(),
           "zero width offset reads zero safely then fails distance validation");

    Fixture overrun(2);
    overrun.single(1); overrun.single(3);
    overrun.literal('A'); overrun.copy(1, 1, 7, 3);
    expect(overrun.bytes, Error::invalid_backreference, a, 12, overrun.bytes.size(),
           "copy cannot exceed declared output even with spare capacity");

    Fixture widest_offset(2);
    widest_offset.single(32); widest_offset.single(1);
    widest_offset.literal('A'); widest_offset.copy(0xffffffffU, 32, 1, 1);
    expect(widest_offset.bytes, Error::invalid_backreference, a, 2, widest_offset.bytes.size(),
           "maximum offset rejected without unsafe pointer formation");
    Fixture widest_length(2);
    widest_length.single(1); widest_length.single(32);
    widest_length.literal('A'); widest_length.copy(1, 1, 0xffffffffU, 32);
    expect(widest_length.bytes, Error::invalid_backreference, a, 2, widest_length.bytes.size(),
           "maximum length checked before copying");

    Fixture sampled_underflow(2, 1);
    sampled_underflow.single(2); sampled_underflow.single(1);
    sampled_underflow.literal('A'); sampled_underflow.bits(1, 1);
    sampled_underflow.bits(0, 2); sampled_underflow.bits(1, 2);
    sampled_underflow.bits(1, 1);
    expect(sampled_underflow.bytes, Error::invalid_backreference, a, 2, sampled_underflow.bytes.size(),
           "sampled distance underflow rejected before subtraction");

    Fixture sampled_huge(2, 255);
    sampled_huge.single(32); sampled_huge.single(1);
    sampled_huge.literal('A'); sampled_huge.copy(0xffffffffU, 32, 1, 1);
    expect(sampled_huge.bytes, Error::invalid_backreference, a, 2, sampled_huge.bytes.size(),
           "sampled distance arithmetic does not wrap at 32 bits");
}

void truncation() {
    Fixture missing_trees(0);
    expect(missing_trees.bytes, Error::truncated, {}, 0, 9, "missing tree flags");
    Fixture missing_width(0);
    missing_width.bits(0, 8);
    expect(missing_width.bytes, Error::truncated, {}, 0, 10, "leaf width truncated within byte");
    Fixture unterminated(0);
    unterminated.end(); unterminated.leaf(0);
    for (unsigned i = 1; i < 4; ++i) {
        unterminated.leaf(0); unterminated.join();
    }
    expect(unterminated.bytes, Error::truncated, {}, 0, unterminated.bytes.size(),
           "second tree terminator absent at exact byte boundary");

    Fixture literals(3);
    literals.end(); literals.end();
    literals.literal('A'); literals.literal('B'); literals.literal('C');
    for (std::size_t size = 10; size < literals.bytes.size(); ++size) {
        const auto count = ((size - 9) * 8 - 2) / 9;
        const auto prefix = text(std::string_view("ABC").substr(0, count));
        expect(std::span<const std::byte>(literals.bytes).first(size), Error::truncated,
               prefix, 3, size, "literal reads stop safely at each byte boundary");
    }

    Fixture copy(4);
    copy.single(32); copy.single(32); copy.literal('Z'); copy.copy(1, 32, 3, 32);
    const auto z = text("Z");
    // Trees plus literal occupy 29 bits; all these cuts occur in the copy samples.
    for (std::size_t size = 13; size < copy.bytes.size(); ++size) {
        expect(std::span<const std::byte>(copy.bytes).first(size), Error::truncated,
               z, 4, size, "truncated offset or length retains only literal prefix");
    }

    Fixture branches(2);
    branches.pair(32, 32); branches.single(1); branches.literal('A');
    branches.bits(1, 1); branches.bits(1, 1); branches.bits(1, 32); branches.bits(1, 1);
    branches.bytes.resize(15);
    const auto a = text("A");
    expect(branches.bytes, Error::truncated, a, 2, branches.bytes.size(),
           "truncated branch-selected sample");

    Fixture missing_branch(2);
    missing_branch.pair(0, 0); missing_branch.single(0);
    missing_branch.literal('A'); missing_branch.bits(1, 1);
    expect(missing_branch.bytes, Error::truncated, a, 2, missing_branch.bytes.size(),
           "branch bit absent at exact byte boundary");

    Fixture second_sample(2, 1);
    second_sample.single(32); second_sample.single(0); second_sample.literal('A');
    second_sample.bits(1, 1); second_sample.bits(1, 32); second_sample.bits(2, 32);
    second_sample.bytes.resize(17);
    expect(second_sample.bytes, Error::truncated, a, 2, second_sample.bytes.size(),
           "sampled residual path requires complete second offset sample");

    Fixture aligned_zero(2);
    aligned_zero.single(1); aligned_zero.pair(0, 0); aligned_zero.literal('A');
    // 39 bits before tokens; three tokens end at exactly 48 bits.
    for (unsigned i = 0; i < 3; ++i) {
        aligned_zero.bits(1, 1); aligned_zero.bits(1, 1); aligned_zero.bits(0, 1);
    }
    expect(aligned_zero.bytes, Error::truncated, a, 2, aligned_zero.bytes.size(),
           "finite zero length tokens eventually report input exhaustion");
}

} // namespace

int main() {
    header_and_literals();
    direct_copies();
    sampled_copies();
    zero_length_tokens();
    tree_limits();
    backreference_errors();
    truncation();
    if (failures != 0) {
        std::cerr << failures << " vpk0 checks failed\n";
        return 1;
    }
    return 0;
}
