#include "35mmisle/rsp.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

namespace {
int failures = 0;
void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

constexpr std::uint32_t reg_op(unsigned function, unsigned rd, unsigned rs, unsigned rt, unsigned shift = 0) {
    return (rs << 21) | (rt << 16) | (rd << 11) | (shift << 6) | function;
}
constexpr std::uint32_t imm_op(unsigned opcode, unsigned rt, unsigned rs, int immediate) {
    return (opcode << 26) | (rs << 21) | (rt << 16) | std::uint16_t(immediate);
}

// These instructions and command streams are original synthetic inputs. No
// ROM fixture, SDK routine or externally assembled microcode is used.
struct Program {
    std::vector<std::byte> bytes;
    void emit(std::uint32_t word) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            bytes.push_back(std::byte((word >> shift) & 255u));
        }
    }
    void li(unsigned reg, std::uint32_t value) {
        emit(0x3c000000u | (reg << 16) | (value >> 16));
        emit(0x34000000u | (reg << 21) | (reg << 16) | (value & 65535u));
    }
    void write(unsigned cp0, std::uint32_t value) {
        li(1, value);
        emit(0x40800000u | (1u << 16) | (cp0 << 11));
    }
    void read(unsigned reg, unsigned cp0) {
        emit(0x40000000u | (reg << 16) | (cp0 << 11));
    }
    void store(unsigned reg, unsigned offset) {
        emit(0xac000000u | (reg << 16) | offset);
    }
    void dma(bool read, unsigned local, unsigned external, std::uint32_t size) {
        write(0, local);
        write(1, external);
        write(read ? 2 : 3, size);
    }
    void stop() { emit(0x0000000du); }
    void pad_to(unsigned pc) {
        while (bytes.size() < pc) emit(0);
    }
};

struct Capture {
    std::array<std::uint32_t, 1024> words{};
    std::array<std::size_t, 128> lengths{};
    std::size_t used = 0;
    std::size_t packets = 0;
    bool reject = false;
    static bool accept(void* opaque, std::span<const std::uint32_t> packet) noexcept {
        auto& self = *static_cast<Capture*>(opaque);
        if (self.reject || packet.size() > self.words.size() - self.used || self.packets == self.lengths.size()) {
            return false;
        }
        std::copy(packet.begin(), packet.end(), self.words.begin() + self.used);
        self.used += packet.size();
        self.lengths[self.packets++] = packet.size();
        return true;
    }
};

isle::rsp::Task task(const Program& program) {
    isle::rsp::Task value{};
    value.type = 1;
    value.flags = 4;
    value.boot_address = 0x1000;
    value.boot_size = static_cast<std::uint32_t>(program.bytes.size());
    value.stack_address = 0x10000;
    value.stack_size = 0x400;
    value.output_address = 0x20000;
    value.output_end = 0x21000;
    value.commands_address = 0x18000;
    return value;
}

void scalar_word_semantics(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    const std::array<std::uint32_t, 4> input{0x7fffffffu, 1u, 0x80000001u, 0xffffffffu};
    std::copy(input.begin(), input.end(), ram.begin() + 0x3000 / 4);
    Program program;
    program.dma(true, 0x100, 0x3000, 15);
    for (unsigned reg = 2; reg != 6; ++reg) {
        program.emit(imm_op(0x23, reg, 0, 0x100 + (reg - 2) * 4)); // LW
    }
    program.li(7, 33);
    struct Case { std::uint32_t instruction, expected; const char* description; };
    // Operands arrive through real DMA/LW, not host-side register inspection.
    // Adjacent dependent cases also catch stale host-width extension facts.
    const Case cases[]{
        {imm_op(0x08, 6, 2, 1), 0x80000000, "ADDI wraps signed overflow to 32 bits"},
        {reg_op(0x2a, 6, 6, 0), 1, "wrapped ADDI result compares as signed negative"},
        {reg_op(0x21, 6, 5, 3), 0, "ADDU wraps carry out of bit 31"},
        {reg_op(0x2b, 6, 6, 3), 1, "wrapped ADDU result compares as unsigned zero"},
        {reg_op(0x22, 6, 0, 3), 0xffffffff, "SUB wraps underflow without trapping"},
        {reg_op(0x23, 6, 4, 5), 0x80000002, "SUBU retains only the low 32 bits"},
        {reg_op(0x2a, 6, 4, 3), 1, "SLT compares bit-31 operands as signed"},
        {reg_op(0x2b, 6, 4, 3), 0, "SLTU compares bit-31 operands as unsigned"},
        {imm_op(0x0a, 6, 4, 0), 1, "SLTI sign-extends its source"},
        {imm_op(0x0b, 6, 4, -1), 1, "SLTIU sign-extends its immediate before unsigned comparison"},
        {imm_op(0x0b, 6, 5, -1), 0, "SLTIU equality with negative immediate is not less-than"},
        {reg_op(0x00, 6, 0, 4, 1), 2, "SLL discards bits shifted past bit 31"},
        {reg_op(0x02, 6, 0, 4, 31), 1, "SRL zero-fills bit-31 source"},
        {reg_op(0x03, 6, 0, 4, 31), 0xffffffff, "SRA sign-fills bit-31 source"},
        {reg_op(0x04, 6, 7, 4), 2, "SLLV masks shift count to five bits"},
        {reg_op(0x06, 6, 7, 4), 0x40000000, "SRLV masks count and zero-fills"},
        {reg_op(0x07, 6, 7, 4), 0xc0000000, "SRAV masks count and sign-fills"},
        {imm_op(0x09, 6, 5, 1), 0, "ADDIU wraps its sign-extended immediate"},
        {reg_op(0x22, 6, 4, 3), 0x80000000, "SUB produces the signed minimum word"},
        {reg_op(0x03, 6, 0, 6, 1), 0xc0000000, "SRA observes sign after preceding arithmetic"}
    };
    unsigned output = 0x300;
    for (const auto& entry : cases) {
        program.emit(entry.instruction);
        program.store(6, output);
        ram[(0x8000 + output - 0x300) / 4] = 0xdeadbeef;
        output += 4;
    }
    program.dma(false, 0x300, 0x8000, output - 0x300 - 1);
    program.stop();
    check(bool(machine.execute(task(program), program.bytes)), "scalar word semantics reach BREAK");
    unsigned index = 0x8000 / 4;
    for (const auto& entry : cases) check(ram[index++] == entry.expected, entry.description);
}

void scalar_memory_semantics(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    Program program;
    program.li(2, 0x80ff7f01);
    program.store(2, 0xffc);
    program.li(2, 0x23456789);
    program.store(2, 0);
    program.li(3, 0x1fff); // Scalar memory always aliases DMEM, never the IMEM bank.
    const std::array loads{
        imm_op(0x20, 6, 3, -3), imm_op(0x24, 6, 3, -3), // LB/LBU
        imm_op(0x21, 6, 3, -3), imm_op(0x25, 6, 3, -3), // LH/LHU
        imm_op(0x21, 6, 3, 0), imm_op(0x25, 6, 3, -2),
        imm_op(0x21, 6, 3, -2), // Unaligned signed halfword.
        imm_op(0x23, 6, 3, -1), imm_op(0x23, 6, 3, 0) // Wrapped unaligned LW.
    };
    unsigned output = 0x300;
    for (const auto instruction : loads) {
        program.emit(instruction);
        program.store(6, output);
        output += 4;
    }
    const auto record = [&](int address) {
        program.emit(imm_op(0x23, 6, 0, address));
        program.store(6, output);
        output += 4;
    };
    program.li(4, 0xa1b2c3d4);
    program.emit(imm_op(0x2b, 4, 3, 0)); // Unaligned SW across the bank end.
    record(0xffc);
    record(0);
    program.li(4, 0xeeff);
    program.emit(imm_op(0x29, 4, 3, 0)); // SH wraps only its second byte.
    record(0xffc);
    record(0);
    program.li(4, 0x5a);
    program.emit(imm_op(0x28, 4, 3, 1)); // SB at 0x2000 aliases DMEM byte 0.
    record(0);
    program.li(0, 0xdeadbeef);
    program.emit(reg_op(0x21, 0, 4, 4));
    program.emit(imm_op(0x23, 0, 3, -1));
    program.store(0, output);
    output += 4;
    program.emit(imm_op(0x09, 6, 0, 0x55));
    program.store(6, output);
    output += 4;
    const std::array<std::uint32_t, 16> expected{
        0xffffff80, 0x80, 0xffff80ff, 0x80ff, 0x123, 0xff7f, 0xffffff7f, 0x7f012345,
        0x01234567, 0x80ff7fa1, 0xb2c3d489, 0x80ff7fee, 0xffc3d489, 0x5ac3d489, 0, 0x55
    };
    std::fill_n(ram.begin() + 0x8000 / 4, expected.size(), 0xdeadbeef);
    program.dma(false, 0x300, 0x8000, output - 0x300 - 1);
    program.stop();
    check(bool(machine.execute(task(program), program.bytes)), "scalar bank-wrap microcode reaches BREAK");
    check(std::equal(expected.begin(), expected.end(), ram.begin() + 0x8000 / 4),
          "scalar loads/stores preserve big-endian extension, unaligned bank wrap and immutable r0");
}

void linked_and_indirect_delays(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    Program program;
    program.li(2, 1);
    program.li(3, 0);
    program.emit(imm_op(4, 0, 2, 2)); // Untaken BEQ still executes its delay slot.
    program.emit(imm_op(9, 3, 3, 1));
    program.emit(imm_op(9, 3, 3, 2));
    program.emit(imm_op(4, 2, 2, 2));
    program.emit(imm_op(9, 2, 0, 0)); // Taken condition must survive overwriting its input.
    program.emit(imm_op(9, 3, 3, 0x40)); // Skipped.
    program.emit(imm_op(9, 3, 3, 4));
    const auto jal_link = static_cast<std::uint32_t>(program.bytes.size()) + 8;
    program.emit(0x0c000040); // JAL 0x100
    program.emit(imm_op(9, 3, 3, 8));
    program.store(3, 0x308);
    program.li(5, 0x180);
    const auto jalr_link = static_cast<std::uint32_t>(program.bytes.size()) + 8;
    program.emit(reg_op(9, 5, 5, 0)); // JALR r5,r5 captures target before writing link.
    program.store(5, 0x30c); // Link is already visible in the delay slot.
    program.store(3, 0x310);
    program.li(6, 0x200);
    program.emit(reg_op(8, 0, 6, 0));
    program.emit(imm_op(9, 6, 0, 0)); // JR must retain target captured before the slot.
    program.stop();
    program.pad_to(0x100);
    program.store(31, 0x300);
    program.store(3, 0x304);
    program.emit(reg_op(8, 0, 31, 0));
    program.emit(imm_op(9, 3, 3, 16));
    program.pad_to(0x180);
    program.emit(reg_op(8, 0, 5, 0));
    program.emit(imm_op(9, 3, 3, 32));
    program.pad_to(0x200);
    program.store(3, 0x314);
    program.dma(false, 0x300, 0x8000, 23);
    program.stop();
    const std::array<std::uint32_t, 6> expected{jal_link, 15, 31, jalr_link, 63, 63};
    std::fill_n(ram.begin() + 0x8000 / 4, expected.size(), 0xdeadbeef);
    check(bool(machine.execute(task(program), program.bytes)), "linked and indirect delay microcode reaches BREAK");
    check(std::equal(expected.begin(), expected.end(), ram.begin() + 0x8000 / 4),
          "taken/untaken slots, PC+8 links and pre-slot indirect targets produce exact scalar trace");
}

void consecutive_branch_service_resume(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    for (unsigned taken = 0; taken != 4; ++taken) {
        machine.reset();
        Program program;
        program.write(0, 0x300);
        program.write(1, 0x8000);
        program.li(1, 7);
        program.li(2, 0);
        program.li(3, (taken & 1u) != 0 ? 0 : 1);
        program.li(4, (taken & 2u) != 0 ? 0 : 1);
        program.li(6, 0x1234abcd);
        program.store(6, 0x300);
        program.store(0, 0x304);
        auto pc = static_cast<unsigned>(program.bytes.size());
        program.emit(imm_op(4, 0, 3, (0x100 - pc - 4) / 4));
        pc = static_cast<unsigned>(program.bytes.size());
        program.emit(imm_op(4, 0, 4, (0x180 - pc - 4) / 4));
        program.emit(imm_op(9, 2, 2, 1)); // Second slot when first branch is untaken.
        program.emit(imm_op(9, 2, 2, 2));
        program.emit(0x08000080); // J 0x200
        program.emit(0);
        program.pad_to(0x100);
        // When both branches are taken, the first target is the second
        // branch's latent delay slot. DMA must complete once, then resume
        // at 0x180, not at this instruction or sequential PC 0x104.
        program.emit(0x40811800); // MTC0 r1,SP_WR_LEN
        program.emit(imm_op(9, 2, 2, 4));
        program.emit(0x08000080);
        program.emit(0);
        program.pad_to(0x180);
        program.emit(imm_op(9, 2, 2, 8));
        program.emit(0x08000080);
        program.emit(0);
        program.pad_to(0x200);
        program.store(2, 0x300);
        program.dma(false, 0x300, 0x8100, 7);
        program.stop();
        ram[0x8000 / 4] = ram[0x8004 / 4] = ram[0x8100 / 4] = 0xdeadbeef;
        const auto result = machine.execute(task(program), program.bytes);
        const std::array<std::uint32_t, 4> expected{3, 4, 9, 8};
        check(bool(result) && result.dma_writes == ((taken & 1u) != 0 ? 2u : 1u),
              "consecutive branch combinations return from service exactly once");
        check(ram[0x8100 / 4] == expected[taken],
              "consecutive branch predicates and service resume retain exact architectural PC");
        check(ram[0x8000 / 4] == ((taken & 1u) != 0 ? 0x1234abcdu : 0xdeadbeefu) &&
              ram[0x8004 / 4] == ((taken & 1u) != 0 ? 0u : 0xdeadbeefu),
              "latent delay DMA publishes its real pre-resume data only when executed");
    }
}

void vector_helper_operands(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    const std::array<std::uint32_t, 8> input{
        0x01234567, 0x89abcdef, 0x10203040, 0x50607080,
        0xfedcba98, 0x76543210, 0x11112222, 0x33334444
    };
    std::copy(input.begin(), input.end(), ram.begin() + 0x3000 / 4);
    Program program;
    program.dma(true, 0x100, 0x3000, 31);
    program.li(2, 0x120);
    program.li(4, 0x13579bdf); // A live scalar must survive the helper calls.
    program.emit(0xc8000000u | (2u << 21) | (3u << 16) | (4u << 11) | 0x7eu); // LQV v3,-2(r2)
    program.emit(0xc8000000u | (2u << 21) | (9u << 16) | (4u << 11) | 0x7fu); // LQV v9,-1(r2)
    program.emit(0x4a000000u | (13u << 21) | (9u << 16) | (3u << 11) | (17u << 6) | 0x2cu);
    // VXOR v17,v3,v9[5]: e=13 broadcasts the independently chosen 0x2222
    // lane. Distinct vd/vs/vt/e expose swapped or lost five-argument ABI operands.
    program.li(2, 0x320);
    program.emit(0xe8000000u | (2u << 21) | (17u << 16) | (4u << 11) | 0x7eu); // SQV v17,-2(r2)
    program.store(4, 0x310);
    program.emit(0x48000000u | (17u << 11) | (4u << 7)); // MFC2 r0,v17[4] must discard.
    program.store(0, 0x314);
    program.emit(0x48000000u | (6u << 16) | (17u << 11) | (4u << 7));
    program.store(6, 0x318);
    program.store(0, 0x31c);
    program.dma(false, 0x300, 0x8000, 31);
    program.stop();
    const std::array<std::uint32_t, 8> expected{
        0x23016745, 0xab89efcd, 0x32021262, 0x724252a2, 0x13579bdf, 0, 0xffffab89, 0
    };
    std::fill_n(ram.begin() + 0x8000 / 4, expected.size(), 0xdeadbeef);
    check(bool(machine.execute(task(program), program.bytes)), "vector helper microcode reaches BREAK");
    check(std::equal(expected.begin(), expected.end(), ram.begin() + 0x8000 / 4),
          "real vector loads/XOR/store preserve all helper operands, signed offsets, scalar state and MFC2 extension");
}

void dma_alignment_count_skip_wrap(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    const std::array<std::uint32_t, 8> input{0x10203040, 0x50607080, 0x90a0b0c0, 0xd0e0f000,
                                              0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00};
    std::copy(input.begin(), input.begin() + 4, ram.begin() + 0x3000 / 4);
    std::copy(input.begin() + 4, input.end(), ram.begin() + 0x3020 / 4);
    std::fill(ram.begin() + 0x4000 / 4, ram.begin() + 0x4030 / 4, 0xcafef00du);
    Program program;
    // Both endpoints align downward; row length rounds to 16, count is two,
    // skip rounds to 16. First row wraps DMEM without crossing into IMEM.
    program.dma(true, 0xfff, 0x3007, (0x17u << 20) | (1u << 12) | 8u);
    program.read(2, 0);
    program.store(2, 0x100);
    program.read(2, 1);
    program.store(2, 0x104);
    program.read(2, 2);
    program.store(2, 0x108);
    program.read(2, 3);
    program.store(2, 0x10c);
    program.dma(false, 0xfff, 0x4007, (0x17u << 20) | (1u << 12) | 8u);
    program.dma(false, 0x100, 0x4100, 15);
    program.stop();
    const auto result = machine.execute(task(program), program.bytes);
    check(bool(result), "aligned count/skip/wrap microcode reaches BREAK");
    check(result.dma_reads == 1 && result.dma_writes == 2 && result.imem_writes == 1,
          "DMA counters distinguish bank reloads from the initial boot write");
    check(std::equal(input.begin(), input.begin() + 4, ram.begin() + 0x4000 / 4) &&
          std::equal(input.begin() + 4, input.end(), ram.begin() + 0x4020 / 4),
          "bidirectional count/skip DMA preserves all words across bank wrap");
    check(ram[0x4010 / 4] == 0xcafef00d && ram[0x401c / 4] == 0xcafef00d,
          "DMA does not write skipped RDRAM rows");
    check(ram[0x4100 / 4] == 0x18 && ram[0x4104 / 4] == 0x3030,
          "CPU observes wrapped same-bank SP address and no skip after final row");
    check(ram[0x4108 / 4] == 0x01000ff8 && ram[0x410c / 4] == 0x01000ff8,
          "both length registers expose finished length/count and retained aligned skip");
}

void programmed_dma_latches(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    for (unsigned endpoint = 0; endpoint != 2; ++endpoint) {
        machine.reset();
        ram[0x3000 / 4] = 0x11223344;
        ram[0x3004 / 4] = 0x55667788;
        ram[0x3008 / 4] = ram[0x300c / 4] = 0;
        ram[0x4000 / 4] = ram[0x4004 / 4] = 0;
        Program program;
        program.dma(true, 0, 0x3000, 7);
        program.li(2, 0xaabbccdd);
        program.store(2, 8);
        program.li(2, 0xeeff0011);
        program.store(2, 12);
        // Reprogram just one pending endpoint. Finished/readback addresses
        // advance, but must not change the other endpoint's programmed latch.
        program.write(endpoint == 0 ? 1u : 0u, endpoint == 0 ? 0x4000u : 8u);
        program.write(3, 7);
        program.stop();
        const auto result = machine.execute(task(program), program.bytes);
        check(bool(result), "single-endpoint DMA reprogramming completes");
        if (endpoint == 0) {
            check(ram[0x4000 / 4] == 0x11223344 && ram[0x4004 / 4] == 0x55667788,
                  "unchanged SP latch remains programmed origin, not finished address");
        } else {
            check(ram[0x3000 / 4] == 0xaabbccdd && ram[0x3004 / 4] == 0xeeff0011,
                  "unchanged RDRAM latch remains programmed origin, not finished address");
        }
    }
}

void retained_single_step(isle::rsp::Machine& machine) {
    machine.reset();
    Program stepping;
    stepping.write(4, 64); // SP_SET_SSTEP is explicitly unsupported.
    stepping.stop();
    const auto first = machine.execute(task(stepping), stepping.bytes);
    check(first.error == isle::rsp::Error::unexpected_stop && (first.sp_status & 32u) != 0,
          "single-step request fails with the unsupported mode retained");
    Program normal;
    normal.stop();
    const auto warm = machine.execute(task(normal), normal.bytes);
    check(warm.error == isle::rsp::Error::unexpected_stop && warm.imem_writes == 0,
          "warm task cannot bypass retained single-step failure or publish boot writes");
    machine.reset();
    check(bool(machine.execute(task(normal), normal.bytes)),
          "explicit cold reset recovers from unsupported single-step mode");
}

void invalid_dma_is_atomic(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    std::fill(ram.begin() + 0x5000 / 4, ram.begin() + 0x5020 / 4, 0xabcdef12u);
    Program program;
    // First row is inside RAM, second row is outside. No row may be copied.
    program.dma(false, 0, 0x7ffff0, (0x100u << 20) | (1u << 12) | 15u);
    program.stop();
    const auto before = ram[0x7ffff0 / 4];
    const auto result = machine.execute(task(program), program.bytes);
    check(result.error == isle::rsp::Error::address_out_of_range && result.dma_writes == 0,
          "multirow out-of-range operand returns explicit DMA failure");
    check(ram[0x7ffff0 / 4] == before, "DMA validates every row before modifying RDRAM");
}

void imem_reload(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    Program replacement;
    replacement.li(2, 0x1234abcdu);
    replacement.store(2, 0x300);
    replacement.dma(false, 0x300, 0x6000, 7);
    replacement.stop();
    for (std::size_t offset = 0; offset != replacement.bytes.size(); offset += 4) {
        std::uint32_t word = 0;
        for (unsigned byte = 0; byte != 4; ++byte) {
            word = (word << 8) | std::to_integer<unsigned>(replacement.bytes[offset + byte]);
        }
        ram[(0x5000 + offset) / 4] = word;
    }
    Program program;
    program.write(0, 0x1100);
    program.write(1, 0x5000);
    program.write(2, static_cast<std::uint32_t>(replacement.bytes.size() - 1));
    program.emit(0x08000040u); // J IMEM 0x100
    program.emit(0);
    while (program.bytes.size() < 0x100) {
        program.emit(0);
    }
    // The region compiled from PC 0 includes these stale words. Executing
    // them after IMEM DMA, instead of replacement words, must be observable.
    program.li(2, 0xdeadbeefu);
    program.store(2, 0x300);
    program.dma(false, 0x300, 0x6000, 7);
    program.stop();
    const auto result = machine.execute(task(program), program.bytes);
    check(bool(result) && result.imem_writes == 2 && result.dma_reads == 1,
          "IMEM DMA returns cooperatively and invalidates compiled entry pointers");
    check(ram[0x6000 / 4] == 0x1234abcd, "CPU-visible word comes from reloaded instructions, never stale JIT");
}

void partial_dp(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram, Capture& capture, bool xbus) {
    machine.reset();
    capture = {};
    std::array<std::uint32_t, 46> expected{};
    expected[0] = 0x0f000000u;
    for (std::size_t index = 1; index != 44; ++index) {
        expected[index] = 0x10000000u + static_cast<std::uint32_t>(index);
    }
    expected[44] = 0x29000000u;
    std::copy(expected.begin(), expected.end(), ram.begin() + 0x7000 / 4);
    Program program;
    const auto start = xbus ? 0x400u : 0x7000u;
    if (xbus) {
        program.dma(true, start, 0x7000, 183);
        program.write(11, 2); // SET_XBUS
    }
    program.write(8, start);
    program.write(9, start + 8);
    program.read(2, 10);
    program.store(2, 0x300);
    // Overwrite the already consumed prefix before publishing the rest.
    // Its private partial-packet copy must survive either source-bank reuse.
    program.li(2, 0xdeadbeefu);
    program.store(2, xbus ? start : 0x200);
    program.store(2, xbus ? start + 4 : 0x204);
    if (!xbus) {
        program.dma(false, 0x200, start, 7);
    }
    program.write(9, start + 184);
    program.read(2, 10);
    program.store(2, 0x304);
    program.dma(false, 0x300, 0x8000, 7);
    program.stop();
    const auto result = machine.execute(task(program), program.bytes);
    check(bool(result) && result.packets == 2 && result.packet_words == 46 && result.full_syncs == 1,
          xbus ? "XBUS partial maximum triangle and FullSync are parsed and counted" :
                 "RDRAM FIFO partial maximum triangle and FullSync are parsed and counted");
    check(capture.packets == 2 && capture.lengths[0] == 44 && capture.lengths[1] == 2 &&
          std::equal(expected.begin(), expected.end(), capture.words.begin()),
          "partial packet copies remain byte-exact after producer recycles the source prefix");
    check(ram[0x8000 / 4] == start + 8 && ram[0x8004 / 4] == start + 184,
          "CPU observes all complete and incomplete packet source bytes consumed");
}

void frozen_dp(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram, Capture& capture) {
    machine.reset();
    capture = {};
    ram[0x7000 / 4] = 0x29000000;
    ram[0x7004 / 4] = 0;
    Program program;
    program.write(11, 8); // SET_FREEZE
    program.write(8, 0x7000);
    program.write(9, 0x7008);
    program.read(2, 11);
    program.store(2, 0x300);
    program.write(11, 4); // CLR_FREEZE, now actually copy the pending transfer.
    program.read(2, 11);
    program.store(2, 0x304);
    program.dma(false, 0x300, 0x8000, 7);
    program.stop();
    const auto result = machine.execute(task(program), program.bytes);
    check(bool(result) && result.full_syncs == 1 && capture.packets == 1,
          "frozen DPC transfer resumes only when freeze is cleared");
    check((ram[0x8000 / 4] & 0x602u) == 0x602u && (ram[0x8004 / 4] & 0x602u) == 0,
          "CPU sees real start/end pending and freeze status transitions");
}

void semaphore_status(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    Program program;
    program.read(2, 7);
    program.store(2, 0x300);
    program.read(2, 7);
    program.store(2, 0x304);
    program.write(7, 99);
    program.read(2, 7);
    program.store(2, 0x308);
    program.write(4, 0x400); // SET_SIG0
    program.write(4, 0x600); // CLR_SIG0 and SET_SIG0 together preserve it.
    program.read(2, 4);
    program.store(2, 0x30c);
    program.write(4, 0x200); // CLR_SIG0
    program.read(2, 4);
    program.store(2, 0x310);
    program.write(4, 0x100); // SET_INTR_BREAK; BREAK must keep status bit.
    program.dma(false, 0x300, 0x8000, 23);
    program.stop();
    const auto result = machine.execute(task(program), program.bytes);
    check(bool(result) && (result.sp_status & 0x43u) == 0x43u,
          "BREAK reports HALT/BROKE and preserves interrupt-on-break status");
    check(ram[0x8000 / 4] == 0 && ram[0x8004 / 4] == 1 && ram[0x8008 / 4] == 0,
          "semaphore reads atomically set and every write clears");
    check((ram[0x800c / 4] & 0x80u) != 0 && (ram[0x8010 / 4] & 0x80u) == 0,
          "paired status clear/set operands and explicit signal clear follow hardware semantics");
}

void rejection_and_limits(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram, Capture& capture) {
    ram[0x7000 / 4] = 0x29000000;
    ram[0x7004 / 4] = 0;
    Program program;
    program.write(8, 0x7000);
    program.write(9, 0x7008);
    program.stop();
    machine.reset();
    capture = {};
    capture.reject = true;
    const auto rejected = machine.execute(task(program), program.bytes);
    check(rejected.error == isle::rsp::Error::sink_rejected && rejected.packets == 0 && rejected.full_syncs == 0,
          "sink rejection returns failure without fabricated FullSync or accepted-packet count");
    machine.reset();
    capture = {};
    const auto capacity = machine.execute(task(program), program.bytes, {1000, 1});
    check(capacity.error == isle::rsp::Error::packet_capacity && capture.packets == 0 && capacity.full_syncs == 0,
          "packet budget includes private partial-word copies and fails before sink invocation");
    machine.reset();
    Program partial;
    ram[0x7000 / 4] = 0x0f000000;
    partial.write(8, 0x7000);
    partial.write(9, 0x7008);
    partial.stop();
    const auto malformed = machine.execute(task(partial), partial.bytes);
    check(malformed.error == isle::rsp::Error::malformed_packet && malformed.packets == 0,
          "BREAK with a retained incomplete packet is not successful task completion");
    machine.reset();
    Program invalid;
    invalid.read(2, 16);
    invalid.stop();
    const auto operand = machine.execute(task(invalid), invalid.bytes);
    check(operand.error == isle::rsp::Error::unexpected_stop,
          "invalid COP0 register operand returns through normal JIT thunk with recorded failure");
}

void bounded_loops_and_fresh_reset(isle::rsp::Machine& machine) {
    for (unsigned kind = 0; kind != 3; ++kind) {
        machine.reset();
        Program loop;
        if (kind == 0) {
            loop.emit(0x08000000u); // Local J to the same compiled region entry.
            loop.emit(0);
        } else if (kind == 1) {
            loop.emit(0x1000ffffu); // Local BEQ r0,r0,-1.
            loop.emit(0);
        } else {
            loop.li(2, 0);
            loop.emit(0x00400008u); // Dynamic JR r2 to PC 0.
            loop.emit(0);
        }
        const auto limited = machine.execute(task(loop), loop.bytes, {7, 100});
        check(limited.error == isle::rsp::Error::execution_budget,
              "all direct/conditional/dynamic internal backward edges cross emitted budget guard");
    }
    machine.reset();
    Program stop;
    stop.stop();
    const auto zero = machine.execute(task(stop), stop.bytes, {0, 100});
    check(zero.error == isle::rsp::Error::execution_budget, "zero entry budget fails before first instruction");
    machine.reset();
    const auto fresh = machine.execute(task(stop), stop.bytes, {1, 100});
    check(bool(fresh) && (fresh.sp_status & 3u) == 3u, "cold reset clears prior budget error and runs fresh microcode");
}

void invalid_task_no_writes(isle::rsp::Machine& machine, std::vector<std::uint32_t>& ram) {
    machine.reset();
    Program stop;
    stop.stop();
    auto bad = task(stop);
    bad.output_end = bad.output_address - 8;
    const auto before = ram[0x7000 / 4];
    const auto result = machine.execute(bad, stop.bytes);
    check(result.error == isle::rsp::Error::invalid_task && result.imem_writes == 0 && ram[0x7000 / 4] == before,
          "invalid canonical task ranges fail before any task or boot writes");
}
} // namespace

int main() {
    std::vector<std::uint32_t> ram(8u * 1024u * 1024u / 4u);
    Capture capture;
    isle::rsp::Machine machine(ram, &Capture::accept, &capture);
    scalar_word_semantics(machine, ram);
    scalar_memory_semantics(machine, ram);
    linked_and_indirect_delays(machine, ram);
    consecutive_branch_service_resume(machine, ram);
    vector_helper_operands(machine, ram);
    dma_alignment_count_skip_wrap(machine, ram);
    programmed_dma_latches(machine, ram);
    retained_single_step(machine);
    invalid_dma_is_atomic(machine, ram);
    imem_reload(machine, ram);
    partial_dp(machine, ram, capture, false);
    partial_dp(machine, ram, capture, true);
    frozen_dp(machine, ram, capture);
    semaphore_status(machine, ram);
    rejection_and_limits(machine, ram, capture);
    bounded_loops_and_fresh_reset(machine);
    invalid_task_no_writes(machine, ram);
    if (failures != 0) {
        std::cerr << failures << " synthetic RSP checks failed\n";
        return 1;
    }
    std::cout << "Original synthetic RSP instruction checks passed\n";
    return 0;
}
