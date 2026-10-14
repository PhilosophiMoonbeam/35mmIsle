#include "35mmisle/rom.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <istream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>

namespace {

using isle::rom::ByteOrder;
using isle::rom::Error;
using isle::rom::Result;

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

void expect_error(const Result& result, Error error, const char* description) {
    check(!static_cast<bool>(result) && result.error == error, description);
}

// Original synthetic image: canonical magic followed by byte (index*37+11)
// modulo 256. Generate on demand, including full-size rejection fixtures;
// no ROM, filesystem access, or image-sized allocation is involved.
class FixtureBuffer final : public std::streambuf {
public:
    FixtureBuffer(std::uint64_t size, ByteOrder order,
                  std::uint64_t fail_at = std::numeric_limits<std::uint64_t>::max())
        : size_(size), order_(order), fail_at_(fail_at) {}

protected:
    std::streamsize xsgetn(char* destination, std::streamsize count) override {
        std::streamsize copied = 0;
        while (copied < count) {
            if (position_ == fail_at_) {
                throw std::runtime_error("synthetic read failure");
            }
            if (position_ == size_) {
                break;
            }
            destination[copied++] = current_byte();
            ++position_;
        }
        return copied;
    }

    int_type underflow() override {
        if (position_ == fail_at_) {
            throw std::runtime_error("synthetic read failure");
        }
        return position_ == size_ ? traits_type::eof() : traits_type::to_int_type(current_byte());
    }

    int_type uflow() override {
        const auto value = underflow();
        if (!traits_type::eq_int_type(value, traits_type::eof())) {
            ++position_;
        }
        return value;
    }

private:
    char current_byte() const {
        std::uint64_t canonical = position_;
        if (order_ == ByteOrder::v64) {
            canonical ^= 1;
        } else if (order_ == ByteOrder::n64) {
            canonical ^= 3;
        }
        constexpr std::array<std::uint8_t, 4> magic{0x80, 0x37, 0x12, 0x40};
        const auto byte = canonical < magic.size()
            ? magic[static_cast<std::size_t>(canonical)]
            : static_cast<std::uint8_t>((canonical & 255) * 37 + 11);
        return static_cast<char>(byte);
    }

    std::uint64_t size_;
    ByteOrder order_;
    std::uint64_t fail_at_;
    std::uint64_t position_ = 0;
};

constexpr std::array<ByteOrder, 3> orders{ByteOrder::z64, ByteOrder::v64, ByteOrder::n64};

void digest_vectors_and_orders() {
    struct Vector {
        std::uint64_t size;
        const char* sha1;
    };
    // Fixed answers computed independently with Python hashlib/OpenSSL,
    // not by this implementation. Lengths exercise one- and two-block SHA-1
    // padding, exact block endings, and multiple normalization-buffer edges.
    constexpr std::array<Vector, 14> vectors{{
        {4, "f834b4f983daad0442642b54eaa503d30489ceb7"},
        {8, "1ec3e8a8059021812d1ab17245a2f3778233325d"},
        {52, "973f15cd74db2fe291edcd5c5c70e322e8639001"},
        {56, "3c71ba42196cf229e7e79360379968cf364b43fe"},
        {60, "430c94be44f9c9645f58e3fe746fae589159368a"},
        {64, "2047c71f2f819ae82550a8c977847a604c26813c"},
        {68, "b3efc16e70749b4300afde69bf9cc7a216a1e36c"},
        {120, "35a793adf32536c16a00eaec11035cbffa4f9850"},
        {128, "1bd3cab0da808a5f1bba0ad6c7c6184ad2cd76e6"},
        {32768, "f8b29016eb8bac8a360ee1a1e11e80ec3a6d9407"},
        {32772, "a2db0bfa8f87c1a599e9677276a5bf45952ab901"},
        {32776, "9640be61ce4296481640c45895e959bf7ba5c6a2"},
        {65540, "d879671368602eebb9d051772aa99c4e5fd31aa6"},
        {98312, "3b4e06ff4edf3ccd442600fa39584bf954bb5791"}
    }};
    for (const auto& vector : vectors) {
        for (const auto order : orders) {
            FixtureBuffer buffer(vector.size, order);
            std::istream input(&buffer);
            const auto result = isle::rom::inspect(input);
            check(static_cast<bool>(result), "complete synthetic image must inspect successfully");
            check(result.identity.order == order, "content magic must select the source byte order");
            check(result.identity.size == vector.size, "inspect must consume and count the complete stream");
            check(isle::rom::digest_hex(result.identity) == vector.sha1,
                  "normalized digest must match the fixed independent answer in every order");
        }
    }
}

void incomplete_and_unrecognized_input() {
    for (const auto order : orders) {
        for (std::uint64_t size = 0; size < 4; ++size) {
            FixtureBuffer buffer(size, order);
            std::istream input(&buffer);
            const auto result = isle::rom::inspect(input);
            expect_error(result, Error::truncated, "empty or incomplete magic header must be truncated");
            check(result.identity.size == size, "incomplete header must report consumed bytes");
        }
        for (const std::uint64_t base : {4ULL, 32772ULL, 65540ULL}) {
            for (std::uint64_t tail = 1; tail <= 3; ++tail) {
                FixtureBuffer buffer(base + tail, order);
                std::istream input(&buffer);
                const auto result = isle::rom::inspect(input);
                expect_error(result, Error::truncated,
                             "incomplete four-byte tail must fail even when v64 pairs are complete");
                check(result.identity.size == base + tail, "incomplete tail must be included in consumed size");
            }
        }
    }
    for (const std::string& bytes : {std::string(4, '\0'), std::string("ABCD"),
                                   std::string("\x80\x37\x12\x41", 4)}) {
        std::istringstream input(bytes);
        expect_error(isle::rom::inspect(input), Error::unsupported_order,
                     "unknown or near-miss magic must not be guessed from a filename");
    }
    FixtureBuffer buffer(7, ByteOrder::z64);
    std::istream input(&buffer);
    expect_error(isle::rom::validate_us(input), Error::truncated,
                 "validation must preserve the underlying incomplete-unit failure");
}

void read_failures_and_exception_masks() {
    for (const std::uint64_t fail_at : {0ULL, 2ULL, 4ULL, 32772ULL, 65540ULL, 98312ULL}) {
        for (const bool throwing : {false, true}) {
            FixtureBuffer buffer(98312, ByteOrder::z64, fail_at);
            std::istream input(&buffer);
            if (throwing) {
                input.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
            }
            expect_error(isle::rom::inspect(input), Error::io,
                         "read failure at the header, body, or EOF probe must be an I/O failure");
        }
    }
    for (const auto order : orders) {
        FixtureBuffer buffer(65540, order);
        std::istream input(&buffer);
        input.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
        const auto result = isle::rom::inspect(input);
        check(static_cast<bool>(result), "ordinary EOF must remain successful with stream exceptions enabled");
        check(isle::rom::digest_hex(result.identity) == "d879671368602eebb9d051772aa99c4e5fd31aa6",
              "exception-enabled EOF must preserve the complete digest");
    }
    {
        FixtureBuffer buffer(3, ByteOrder::z64);
        std::istream input(&buffer);
        input.exceptions(std::ios::failbit | std::ios::eofbit);
        expect_error(isle::rom::inspect(input), Error::truncated,
                     "exception-enabled incomplete header must remain truncated rather than I/O");
    }
    for (const auto order : orders) {
        FixtureBuffer buffer(65543, order);
        std::istream input(&buffer);
        input.exceptions(std::ios::failbit | std::ios::eofbit);
        expect_error(isle::rom::inspect(input), Error::truncated,
                     "exception-enabled incomplete tail must remain truncated rather than I/O");
    }
    for (const auto state : {std::ios::failbit, std::ios::badbit, std::ios::badbit | std::ios::eofbit}) {
        std::istringstream input("unused");
        input.setstate(state);
        expect_error(isle::rom::inspect(input), Error::io, "preexisting failed stream must not be accepted");
    }
    FixtureBuffer buffer(65540, ByteOrder::v64, 32772);
    std::istream input(&buffer);
    expect_error(isle::rom::validate_us(input), Error::io, "validation must not replace I/O errors with size errors");
}

void exact_us_rejection_and_extra_data() {
    for (const auto order : orders) {
        FixtureBuffer buffer(98312, order);
        std::istream input(&buffer);
        const auto result = isle::rom::validate_us(input);
        expect_error(result, Error::wrong_size, "complete but wrong-sized input must not validate as US");
        check(result.identity.size == 98312, "size rejection must retain the observed full size");
        check(isle::rom::digest_hex(result.identity) == "3b4e06ff4edf3ccd442600fa39584bf954bb5791",
              "size rejection must still hash the whole normalized stream");
    }
    {
        FixtureBuffer buffer(isle::rom::us_size, ByteOrder::n64);
        std::istream input(&buffer);
        const auto result = isle::rom::validate_us(input);
        expect_error(result, Error::wrong_identity, "exact-sized synthetic input must fail the canonical US digest");
        check(result.identity.size == 16777216, "wrong-identity result must retain exact observed size");
        check(isle::rom::digest_hex(result.identity) == "e49389577f9be7557900c7b5c6669b475d694377",
              "wrong-identity result must hash the entire full-size normalized fixture");
    }
    {
        FixtureBuffer buffer(isle::rom::us_size + 4, ByteOrder::z64);
        std::istream input(&buffer);
        const auto result = isle::rom::validate_us(input);
        expect_error(result, Error::wrong_size, "extra aligned data after the expected image length must be rejected");
        check(result.identity.size == 16777220, "extra trailing word must be counted rather than ignored");
        check(isle::rom::digest_hex(result.identity) == "958ff4085128d07ea9dd5589140060fa8b945a59",
              "extra trailing word must be included in the normalized hash");
    }
}

} // namespace

int main() {
    try {
        digest_vectors_and_orders();
        incomplete_and_unrecognized_input();
        read_failures_and_exception_masks();
        exact_us_rejection_and_extra_data();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: unexpected test exception: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "FAIL: unexpected non-standard test exception\n";
        return 1;
    }
    if (failures != 0) {
        std::cerr << failures << " ROM behavioral check(s) failed.\n";
        return 1;
    }
    std::cout << "ROM identity behavioral checks passed using synthetic inputs only.\n";
    return 0;
}
