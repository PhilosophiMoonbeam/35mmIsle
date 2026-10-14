#include "35mmisle/rom.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

#ifdef _WIN32
using Argument = wchar_t;
constexpr std::wstring_view help_argument = L"--help";
constexpr std::wstring_view check_argument = L"check";
#else
using Argument = char;
constexpr std::string_view help_argument = "--help";
constexpr std::string_view check_argument = "check";
#endif

void usage(std::ostream& output) {
    output << "Usage: 35mmisle-rom check <path>\n"
              "       35mmisle-rom --help\n"
              "Read-only identity check for the exact US Pokemon Snap ROM.\n"
              "Content determines z64/v64/n64 byte order; hashing normalizes in memory.\n"
              "This tool does not launch a game, extract payloads, or write ROM data.\n";
}

const char* order_name(isle::rom::ByteOrder order) noexcept {
    switch (order) {
    case isle::rom::ByteOrder::z64: return "z64";
    case isle::rom::ByteOrder::v64: return "v64";
    case isle::rom::ByteOrder::n64: return "n64";
    }
    return "unknown";
}

int run(int argc, Argument* argv[]) {
    if (argc == 2 && std::basic_string_view<Argument>(argv[1]) == help_argument) {
        usage(std::cout);
        std::cout.flush();
        if (!std::cout) {
            std::cerr << "Cannot write help output.\n";
            return 1;
        }
        return 0;
    }
    if (argc != 3 || std::basic_string_view<Argument>(argv[1]) != check_argument) {
        std::cerr << "Invalid arguments.\n";
        usage(std::cerr);
        return 2;
    }

    // Native filesystem paths preserve Linux argv bytes and Windows wmain's
    // UTF-16 input. Never narrow a Windows filename through a legacy code page.
    std::ifstream input(std::filesystem::path(argv[2]), std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "ROM identity check failed: cannot open input for reading.\n";
        return 1;
    }
    const auto result = isle::rom::validate_us(input);
    if (!result) {
        std::cerr << "ROM identity check failed: ";
        switch (result.error) {
        case isle::rom::Error::io:
            std::cerr << "input I/O failure.\n";
            break;
        case isle::rom::Error::truncated:
            std::cerr << "incomplete magic header or four-byte unit.\n";
            break;
        case isle::rom::Error::unsupported_order:
            std::cerr << "unrecognized content magic.\n";
            break;
        case isle::rom::Error::wrong_size:
            std::cerr << "wrong size; observed " << result.identity.size
                      << " bytes, expected " << isle::rom::us_size
                      << "; normalized SHA1 " << isle::rom::digest_hex(result.identity) << ".\n";
            break;
        case isle::rom::Error::wrong_identity:
            std::cerr << "normalized SHA1 does not match the exact US image; observed "
                      << isle::rom::digest_hex(result.identity) << ".\n";
            break;
        case isle::rom::Error::none:
            std::cerr << "inconsistent result.\n";
            break;
        }
        return 1;
    }

    std::cout << "Identity-only check: accepted exact US ROM; not a game launch.\n"
              << "byteorder: " << order_name(result.identity.order) << '\n'
              << "size: " << result.identity.size << " bytes\n"
              << "normalized SHA1: " << isle::rom::digest_hex(result.identity) << '\n';
    std::cout.flush();
    if (!std::cout) {
        std::cerr << "ROM identity check failed: cannot write identity report.\n";
        return 1;
    }
    return 0;
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    try {
        return run(argc, argv);
    } catch (...) {
        // Exception messages from the filesystem may contain private paths.
        std::cerr << "ROM identity check failed: input or reporting operation failed.\n";
        return 1;
    }
}
