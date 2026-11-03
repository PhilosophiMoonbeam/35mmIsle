// SPDX-License-Identifier: MIT
// Copyright (c) 2026 35mmIsle contributors
// Region discovery and the RSP instruction/helper mapping follow MIT-selected
// parallel-rsp, copyright (c) 2020 Hans-Kristian Arntzen. No lightning code.
#include "rsp_llvm.hpp"
#include "rsp_op.hpp"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/TargetSelect.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace RSP::LLVM {
namespace {
namespace ir = ::llvm;
namespace orc = ::llvm::orc;
using Entry = std::uint32_t (*)(CPUState*);

// Only these explicitly bound host functions are visible to generated code.
#define VECTOR_HELPERS(X) \
    X(VMULF) X(VMULU) X(VRNDP) X(VMULQ) X(VMUDL) X(VMUDM) X(VMUDN) X(VMUDH) \
    X(VMACF) X(VMACU) X(VRNDN) X(VMACQ) X(VMADL) X(VMADM) X(VMADN) X(VMADH) \
    X(VADD) X(VSUB) X(VABS) X(VADDC) X(VSUBC) X(VSAR) X(VLT) X(VEQ) X(VNE) \
    X(VGE) X(VCL) X(VCH) X(VCR) X(VMRG) X(VAND) X(VNAND) X(VOR) X(VNOR) \
    X(VXOR) X(VNXOR) X(VRCP) X(VRCPL) X(VRCPH) X(VMOV) X(VRSQ) X(VRSQL) \
    X(VRSQH) X(VNOP) X(RESERVED)
#define MEMORY_HELPERS(X) \
    X(LBV) X(LSV) X(LLV) X(LDV) X(LQV) X(LRV) X(LPV) X(LUV) X(LHV) X(LFV) X(LTV) \
    X(SBV) X(SSV) X(SLV) X(SDV) X(SQV) X(SRV) X(SPV) X(SUV) X(SHV) X(SFV) X(SWV) X(STV)

constexpr const char* vector_ops[64] = {
    "VMULF", "VMULU", "VRNDP", "VMULQ", "VMUDL", "VMUDM", "VMUDN", "VMUDH",
    "VMACF", "VMACU", "VRNDN", "VMACQ", "VMADL", "VMADM", "VMADN", "VMADH",
    "VADD", "VSUB", "RESERVED", "VABS", "VADDC", "VSUBC", "RESERVED", "RESERVED",
    "RESERVED", "RESERVED", "RESERVED", "RESERVED", "RESERVED", "VSAR", "RESERVED", "RESERVED",
    "VLT", "VEQ", "VNE", "VGE", "VCL", "VCH", "VCR", "VMRG",
    "VAND", "VNAND", "VOR", "VNOR", "VXOR", "VNXOR", "RESERVED", "RESERVED",
    "VRCP", "VRCPL", "VRCPH", "VMOV", "VRSQ", "VRSQL", "VRSQH", "VNOP",
    "RESERVED", "RESERVED", "RESERVED", "RESERVED", "RESERVED", "RESERVED", "RESERVED", "VNOP"
};
constexpr const char* load_ops[12] = {
    "LBV", "LSV", "LLV", "LDV", "LQV", "LRV", "LPV", "LUV", "LHV", "LFV", nullptr, "LTV"
};
constexpr const char* store_ops[12] = {
    "SBV", "SSV", "SLV", "SDV", "SQV", "SRV", "SPV", "SUV", "SHV", "SFV", "SWV", "STV"
};

bool is_branch(std::uint32_t instruction) {
    const auto op = instruction >> 26;
    return (op >= 1 && op <= 7) || ((instruction & 0xfc00003eu) == 8);
}

// Keep the existing 256-byte-bank/two-bank region boundaries and logical end
// analysis: they determine budget entry counts, not emulated cycles.
unsigned region_end(const std::uint32_t* code, unsigned start) {
    const unsigned bound = std::min(((start * 4 + CODE_BLOCK_SIZE * 2) >> CODE_BLOCK_SIZE_LOG2)
                                       << (CODE_BLOCK_SIZE_LOG2 - 2), unsigned(IMEM_WORDS));
    unsigned reach = start;
    for (unsigned pc = start; pc < bound; ++pc) {
        const auto instruction = code[pc];
        const auto op = instruction >> 26;
        const bool forward = pc + 1 < reach;
        if (!forward)
            reach = pc + 1;
        if ((instruction >> 25) == 0x25)
            continue;
        if (op == 0) {
            const auto fn = instruction & 63;
            if (!forward && (fn == 8 || fn == 9)) {
                reach = std::max(pc + 2, reach);
                break;
            }
            if (!forward && fn == 13)
                break;
        } else if (op == 2 || op == 3) {
            const auto target = instruction & 1023;
            if (!forward) {
                reach = std::max(pc + 2, reach);
                break;
            }
            if (target >= start && target < bound)
                reach = std::max(reach, target + 1);
        } else if ((op >= 4 && op <= 7) ||
                   (op == 1 && (((instruction >> 16) & 31) == 0 ||
                                ((instruction >> 16) & 31) == 1 ||
                                ((instruction >> 16) & 31) == 16 ||
                                ((instruction >> 16) & 31) == 17))) {
            const auto target = (pc + 1 + instruction) & 1023;
            if (target >= start && target < bound)
                reach = std::max(reach, target + 1);
        }
    }
    return std::min(reach, bound);
}

std::uint64_t code_hash(const std::uint32_t* words, unsigned count) {
    std::uint64_t hash = 0xcbf29ce484222325ull;
    for (unsigned i = 0; i < count; ++i)
        hash = (hash * 0x100000001b3ull) ^ words[i];
    return hash;
}

class RegionIR {
public:
    RegionIR(ir::Module& module, const std::string& name, unsigned start,
             const std::vector<std::uint32_t>& code)
        : module_(module), builder_(module.getContext()), start_(start), code_(code) {
        auto* signature = ir::FunctionType::get(builder_.getInt32Ty(), {builder_.getPtrTy()}, false);
        function_ = ir::Function::Create(signature, ir::GlobalValue::ExternalLinkage, name, module_);
        function_->addFnAttr(ir::Attribute::NoUnwind);
        state_ = function_->getArg(0);
        auto* entry = block("entry");
        builder_.SetInsertPoint(entry);
        pending_ = builder_.CreateAlloca(builder_.getInt1Ty());
        target_ = builder_.CreateAlloca(builder_.getInt32Ty());
        external_ = builder_.CreateAlloca(builder_.getInt1Ty());
        builder_.CreateStore(builder_.CreateICmpNE(load(offsetof(CPUState, has_delay_slot)), word(0)), pending_);
        builder_.CreateStore(load(offsetof(CPUState, branch_target)), target_);
        builder_.CreateStore(builder_.getTrue(), external_);

        entries_.resize(code_.size(), false);
        entries_[0] = true;
        for (unsigned i = 0; i < code_.size(); ++i) {
            const auto instruction = code_[i];
            const auto op = instruction >> 26;
            unsigned target = IMEM_WORDS;
            if (op == 2 || op == 3)
                target = instruction & 1023;
            else if ((op >= 4 && op <= 7) ||
                     (op == 1 && (((instruction >> 16) & 31) == 0 ||
                                  ((instruction >> 16) & 31) == 1 ||
                                  ((instruction >> 16) & 31) == 16 ||
                                  ((instruction >> 16) & 31) == 17)))
                target = (start_ + i + 1 + instruction) & 1023;
            if (target >= start_ && target < start_ + code_.size())
                entries_[target - start_] = true;
            if (i >= 2 && is_branch(code_[i - 2]))
                entries_[i] = true;
            instructions_.push_back(block("pc" + std::to_string((start_ + i) * 4)));
        }
        builder_.CreateBr(instructions_[0]);
        for (unsigned i = 0; i < code_.size(); ++i)
            emit(i);
    }

private:
    ir::Module& module_;
    ir::IRBuilder<> builder_;
    unsigned start_;
    const std::vector<std::uint32_t>& code_;
    ir::Function* function_ = nullptr;
    ir::Value* state_ = nullptr;
    ir::AllocaInst* pending_ = nullptr;
    ir::AllocaInst* target_ = nullptr;
    ir::AllocaInst* external_ = nullptr;
    std::vector<bool> entries_;
    std::vector<ir::BasicBlock*> instructions_;

    ir::ConstantInt* word(std::uint32_t value) { return builder_.getInt32(value); }
    ir::BasicBlock* block(const std::string& name) {
        return ir::BasicBlock::Create(module_.getContext(), name, function_);
    }
    ir::Value* field(std::size_t offset) {
        return builder_.CreateConstGEP1_64(builder_.getInt8Ty(), state_, offset);
    }
    ir::Value* load(std::size_t offset) { return builder_.CreateLoad(builder_.getInt32Ty(), field(offset)); }
    void store(std::size_t offset, ir::Value* value) { builder_.CreateStore(value, field(offset)); }
    ir::Value* reg(unsigned index) {
        return index == 0 ? word(0) : load(offsetof(CPUState, sr) + index * sizeof(std::uint32_t));
    }
    void reg(unsigned index, ir::Value* value) {
        if (index)
            store(offsetof(CPUState, sr) + index * sizeof(std::uint32_t), value);
    }
    ir::Value* integer(ir::Value* condition) { return builder_.CreateZExt(condition, builder_.getInt32Ty()); }
    void finish(ir::Value* pc, ir::Value* mode) {
        store(offsetof(CPUState, pc), builder_.CreateAnd(pc, word(0xffc)));
        store(offsetof(CPUState, has_delay_slot), integer(builder_.CreateLoad(builder_.getInt1Ty(), pending_)));
        store(offsetof(CPUState, branch_target), builder_.CreateLoad(builder_.getInt32Ty(), target_));
        builder_.CreateRet(mode);
    }
    ir::Value* helper(const char* name, bool result, std::initializer_list<ir::Value*> arguments) {
        std::vector<ir::Type*> types(arguments.size(), builder_.getInt32Ty());
        types[0] = builder_.getPtrTy();
        auto signature = ir::FunctionType::get(result ? builder_.getInt32Ty() : builder_.getVoidTy(), types, false);
        auto callee = module_.getOrInsertFunction(std::string("RSP_") + name, signature);
        auto* call = builder_.CreateCall(callee, ir::ArrayRef<ir::Value*>(arguments.begin(), arguments.size()));
        call->setDoesNotThrow();
        return call;
    }
    ir::Value* memory_pointer(ir::Value* address, unsigned bytes) {
        auto* dmem = builder_.CreateLoad(builder_.getPtrTy(), field(offsetof(CPUState, dmem)));
        auto* offset = builder_.CreateXor(builder_.CreateAnd(address, word(4095)), word(4 - bytes));
        return builder_.CreateGEP(builder_.getInt8Ty(), dmem, offset);
    }
    ir::Value* memory_load(ir::Value* address, unsigned bytes, bool sign) {
        ir::Value* value;
        if (bytes == 1) {
            value = builder_.CreateZExt(builder_.CreateLoad(builder_.getInt8Ty(), memory_pointer(address, 1)),
                                       builder_.getInt32Ty());
        } else {
            auto* aligned = block("aligned_load");
            auto* unaligned = block("unaligned_load");
            auto* merged = block("loaded");
            builder_.CreateCondBr(builder_.CreateICmpEQ(builder_.CreateAnd(address, word(bytes - 1)), word(0)),
                                  aligned, unaligned);
            builder_.SetInsertPoint(aligned);
            auto* native = builder_.CreateLoad(builder_.getIntNTy(bytes * 8), memory_pointer(address, bytes));
            native->setAlignment(ir::Align(bytes));
            auto* extended = builder_.CreateZExtOrTrunc(native, builder_.getInt32Ty());
            builder_.CreateBr(merged);
            builder_.SetInsertPoint(unaligned);
            ir::Value* assembled = word(0);
            for (unsigned i = 0; i < bytes; ++i) {
                auto* byte = builder_.CreateLoad(builder_.getInt8Ty(), memory_pointer(builder_.CreateAdd(address, word(i)), 1));
                assembled = builder_.CreateOr(builder_.CreateShl(assembled, word(8)),
                                               builder_.CreateZExt(byte, builder_.getInt32Ty()));
            }
            builder_.CreateBr(merged);
            builder_.SetInsertPoint(merged);
            auto* phi = builder_.CreatePHI(builder_.getInt32Ty(), 2);
            phi->addIncoming(extended, aligned);
            phi->addIncoming(assembled, unaligned);
            value = phi;
        }
        if (sign && bytes < 4)
            value = builder_.CreateSExt(builder_.CreateTrunc(value, builder_.getIntNTy(bytes * 8)), builder_.getInt32Ty());
        return value;
    }
    void memory_store(ir::Value* address, unsigned bytes, ir::Value* value) {
        if (bytes == 1) {
            builder_.CreateStore(builder_.CreateTrunc(value, builder_.getInt8Ty()), memory_pointer(address, 1));
            return;
        }
        auto* aligned = block("aligned_store");
        auto* unaligned = block("unaligned_store");
        auto* merged = block("stored");
        builder_.CreateCondBr(builder_.CreateICmpEQ(builder_.CreateAnd(address, word(bytes - 1)), word(0)),
                              aligned, unaligned);
        builder_.SetInsertPoint(aligned);
        auto* native = builder_.CreateStore(builder_.CreateTruncOrBitCast(value, builder_.getIntNTy(bytes * 8)),
                                           memory_pointer(address, bytes));
        native->setAlignment(ir::Align(bytes));
        builder_.CreateBr(merged);
        builder_.SetInsertPoint(unaligned);
        for (unsigned i = 0; i < bytes; ++i) {
            auto* byte = builder_.CreateTrunc(builder_.CreateLShr(value, word((bytes - i - 1) * 8)), builder_.getInt8Ty());
            builder_.CreateStore(byte, memory_pointer(builder_.CreateAdd(address, word(i)), 1));
        }
        builder_.CreateBr(merged);
        builder_.SetInsertPoint(merged);
    }

    void emit(unsigned i) {
        const unsigned pc = (start_ + i) * 4;
        const auto instruction = code_[i];
        const auto op = instruction >> 26;
        const auto rs = (instruction >> 21) & 31;
        const auto rt = (instruction >> 16) & 31;
        const auto rd = (instruction >> 11) & 31;
        const auto shift = (instruction >> 6) & 31;
        const auto immediate = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(instruction)));
        builder_.SetInsertPoint(instructions_[i]);
        if (entries_[i]) {
            auto* budget = load(offsetof(CPUState, block_budget));
            auto* execute = block("budget_ok");
            auto* exhausted = block("budget_exhausted");
            builder_.CreateCondBr(builder_.CreateICmpNE(budget, word(0)), execute, exhausted);
            builder_.SetInsertPoint(exhausted);
            finish(word(pc), word(MODE_BUDGET));
            builder_.SetInsertPoint(execute);
            store(offsetof(CPUState, block_budget), builder_.CreateSub(budget, word(1)));
        }
        // Capture both pending operands before executing the delay instruction:
        // JALR rs==rd and branches in delay slots must not overwrite them.
        auto* old_pending = builder_.CreateLoad(builder_.getInt1Ty(), pending_);
        auto* old_target = builder_.CreateLoad(builder_.getInt32Ty(), target_);
        auto* old_external = builder_.CreateLoad(builder_.getInt1Ty(), external_);
        ir::Value* taken = builder_.getFalse();
        ir::Value* destination = word(0);
        ir::Value* exit_mode = nullptr;
        bool branch = false;
        bool indirect = false;
        bool known = true;
        auto conditional = [&](ir::Value* condition) {
            branch = true;
            taken = condition;
            destination = word((pc + 4 + (instruction << 2)) & 0xffc);
        };
        auto compare = [&](ir::CmpInst::Predicate predicate, ir::Value* a, ir::Value* b) {
            return builder_.CreateICmp(predicate, a, b);
        };
        if ((instruction >> 25) == 0x25) {
            helper(vector_ops[instruction & 63], false,
                   {state_, word(shift), word(rd), word(rt), word((instruction >> 21) & 15)});
        } else switch (op) {
        case 0:
            switch (instruction & 63) {
            case 0: reg(rd, builder_.CreateShl(reg(rt), word(shift))); break;
            case 2: reg(rd, builder_.CreateLShr(reg(rt), word(shift))); break;
            case 3: reg(rd, builder_.CreateAShr(reg(rt), word(shift))); break;
            case 4: reg(rd, builder_.CreateShl(reg(rt), builder_.CreateAnd(reg(rs), word(31)))); break;
            case 6: reg(rd, builder_.CreateLShr(reg(rt), builder_.CreateAnd(reg(rs), word(31)))); break;
            case 7: reg(rd, builder_.CreateAShr(reg(rt), builder_.CreateAnd(reg(rs), word(31)))); break;
            case 8:
            case 9:
                branch = indirect = true;
                taken = builder_.getTrue();
                destination = builder_.CreateAnd(reg(rs), word(0xffc));
                if ((instruction & 63) == 9)
                    reg(rd, word((pc + 8) & 0xffc));
                break;
            case 13: exit_mode = word(MODE_BREAK); break;
            case 32: case 33: reg(rd, builder_.CreateAdd(reg(rs), reg(rt))); break;
            case 34: case 35: reg(rd, builder_.CreateSub(reg(rs), reg(rt))); break;
            case 36: reg(rd, builder_.CreateAnd(reg(rs), reg(rt))); break;
            case 37: reg(rd, builder_.CreateOr(reg(rs), reg(rt))); break;
            case 38: reg(rd, builder_.CreateXor(reg(rs), reg(rt))); break;
            case 39: reg(rd, builder_.CreateNot(builder_.CreateOr(reg(rs), reg(rt)))); break;
            case 42: reg(rd, integer(compare(ir::CmpInst::ICMP_SLT, reg(rs), reg(rt)))); break;
            case 43: reg(rd, integer(compare(ir::CmpInst::ICMP_ULT, reg(rs), reg(rt)))); break;
            default: known = false; break;
            }
            break;
        case 1:
            if (rt == 0 || rt == 16)
                conditional(compare(ir::CmpInst::ICMP_SLT, reg(rs), word(0)));
            else if (rt == 1 || rt == 17)
                conditional(compare(ir::CmpInst::ICMP_SGE, reg(rs), word(0)));
            else
                known = false;
            if (rt == 16 || rt == 17)
                reg(31, word((pc + 8) & 0xffc));
            break;
        case 2: case 3:
            branch = true;
            taken = builder_.getTrue();
            destination = word((instruction & 1023) * 4);
            if (op == 3)
                reg(31, word((pc + 8) & 0xffc));
            break;
        case 4: conditional(compare(ir::CmpInst::ICMP_EQ, reg(rs), reg(rt))); break;
        case 5: conditional(compare(ir::CmpInst::ICMP_NE, reg(rs), reg(rt))); break;
        case 6: conditional(compare(ir::CmpInst::ICMP_SLE, reg(rs), word(0))); break;
        case 7: conditional(compare(ir::CmpInst::ICMP_SGT, reg(rs), word(0))); break;
        case 8: case 9: reg(rt, builder_.CreateAdd(reg(rs), word(immediate))); break;
        case 10: reg(rt, integer(compare(ir::CmpInst::ICMP_SLT, reg(rs), word(immediate)))); break;
        case 11: reg(rt, integer(compare(ir::CmpInst::ICMP_ULT, reg(rs), word(immediate)))); break;
        case 12: reg(rt, builder_.CreateAnd(reg(rs), word(instruction & 65535))); break;
        case 13: reg(rt, builder_.CreateOr(reg(rs), word(instruction & 65535))); break;
        case 14: reg(rt, builder_.CreateXor(reg(rs), word(instruction & 65535))); break;
        case 15: reg(rt, word(instruction << 16)); break;
        case 16:
            if (rs == 0)
                exit_mode = helper("MFC0", true, {state_, word(rt), word(rd)});
            else if (rs == 4)
                exit_mode = helper("MTC0", true, {state_, word(rd), word(rt)});
            else
                known = false;
            break;
        case 18:
            switch (rs) {
            case 0: helper("MFC2", false, {state_, word(rt), word(rd), word((instruction >> 7) & 15)}); break;
            case 2: helper("CFC2", false, {state_, word(rt), word(rd)}); break;
            case 4: helper("MTC2", false, {state_, word(rt), word(rd), word((instruction >> 7) & 15)}); break;
            case 6: helper("CTC2", false, {state_, word(rt), word(rd)}); break;
            default: known = false; break;
            }
            break;
        case 32: case 33: case 35: case 36: case 37: case 39: {
            const unsigned bytes = op == 32 || op == 36 ? 1 : op == 33 || op == 37 ? 2 : 4;
            if (rt)
                reg(rt, memory_load(builder_.CreateAdd(reg(rs), word(immediate)), bytes, op == 32 || op == 33));
            break;
        }
        case 40: case 41: case 43: {
            const unsigned bytes = op == 40 ? 1 : op == 41 ? 2 : 4;
            memory_store(builder_.CreateAdd(reg(rs), word(immediate)), bytes, reg(rt));
            break;
        }
        case 50: case 58: {
            const char* name = rd < 12 ? (op == 50 ? load_ops[rd] : store_ops[rd]) : nullptr;
            if (!name) {
                known = false;
                break;
            }
            const auto offset = static_cast<std::int32_t>(instruction & 127) - ((instruction & 64) ? 128 : 0);
            helper(name, false, {state_, word(rt), word((instruction >> 7) & 15), word(static_cast<std::uint32_t>(offset)), word(rs)});
            break;
        }
        default: known = false; break;
        }
        if (!known) {
            // Unknown scalar/transfer encodings stop at the faulting instruction,
            // rather than silently turning corrupt microcode into successful work.
            finish(word(pc), word(MODE_HOST_ERROR));
            return;
        }
        builder_.CreateStore(taken, pending_);
        builder_.CreateStore(destination, target_);
        builder_.CreateStore(builder_.getInt1(indirect), external_);
        auto* next_pc = builder_.CreateSelect(old_pending, old_target, word((pc + 4) & 0xffc));
        if (exit_mode) {
            auto* stopped = block("host_exit");
            auto* continued = block("host_continue");
            builder_.CreateCondBr(builder_.CreateICmpNE(exit_mode, word(MODE_CONTINUE)), stopped, continued);
            builder_.SetInsertPoint(stopped);
            finish(next_pc, exit_mode);
            builder_.SetInsertPoint(continued);
        }
        auto* resolve = block("resolve_delay");
        auto* sequential = block("sequential");
        builder_.CreateCondBr(old_pending, resolve, sequential);
        builder_.SetInsertPoint(resolve);
        // A latent entry or a second branch in a delay slot must return to the
        // dispatcher with the new pending branch intact. Indirect transfers
        // retain the existing external-entry budget behavior even for local PCs.
        if (i == 0 || branch) {
            finish(old_target, word(MODE_CONTINUE));
        } else {
            auto* local = block("local_transfer");
            auto* outside = block("external_transfer");
            builder_.CreateCondBr(old_external, outside, local);
            builder_.SetInsertPoint(local);
            auto* dispatch = builder_.CreateSwitch(old_target, outside);
            for (unsigned j = 0; j < code_.size(); ++j)
                if (entries_[j])
                    dispatch->addCase(word((start_ + j) * 4), instructions_[j]);
            builder_.SetInsertPoint(outside);
            finish(old_target, word(MODE_CONTINUE));
        }
        builder_.SetInsertPoint(sequential);
        if (i + 1 < code_.size())
            builder_.CreateBr(instructions_[i + 1]);
        else
            finish(word((pc + 4) & 0xffc), word(MODE_CONTINUE));
    }
};

void optimize(ir::Module& module) {
    ir::LoopAnalysisManager loops;
    ir::FunctionAnalysisManager functions;
    ir::CGSCCAnalysisManager cgscc;
    ir::ModuleAnalysisManager modules;
    ir::PassBuilder passes;
    passes.registerModuleAnalyses(modules);
    passes.registerCGSCCAnalyses(cgscc);
    passes.registerFunctionAnalyses(functions);
    passes.registerLoopAnalyses(loops);
    passes.crossRegisterProxies(loops, functions, cgscc, modules);
    // O1 reduces measured first-use cost for small regions; native codegen
    // retains LLJIT's default optimization level.
    auto pipeline = passes.buildPerModuleDefaultPipeline(ir::OptimizationLevel::O1);
    pipeline.run(module, modules);
}

// Foreign native entry points do not have a Clang C++ function-type cookie.
#if defined(__clang__)
__attribute__((no_sanitize("function")))
#endif
std::uint32_t enter(Entry entry, CPUState* state) noexcept {
    return entry(state);
}
} // namespace

struct CPU::Impl {
    CPUState state{};
    std::unique_ptr<orc::LLJIT> jit;
    struct Region {
        std::uint64_t hash;
        std::vector<std::uint32_t> words;
        Entry entry = nullptr;
    };
    std::array<std::vector<std::unique_ptr<Region>>, IMEM_WORDS> cache;
    std::array<Entry, IMEM_WORDS> active{};
    const std::uint32_t* active_imem = nullptr;
    std::uint64_t serial = 0;

    void initialize() {
        if (jit)
            return;
        static std::once_flag initialized;
        std::call_once(initialized, [] {
            if (ir::InitializeNativeTarget() || ir::InitializeNativeTargetAsmPrinter())
                throw std::runtime_error("LLVM native RSP target initialization failed");
        });
        auto result = orc::LLJITBuilder()
            .setLinkProcessSymbolsByDefault(false)
            .setPlatformSetUp([](orc::LLJIT&) -> ir::Expected<orc::JITDylibSP> {
                // Regions contain no constructors, destructors or ORC runtime.
                return orc::JITDylibSP();
            })
            .setObjectLinkingLayerCreator([](orc::ExecutionSession& session)
                -> ir::Expected<std::unique_ptr<orc::ObjectLayer>> {
                // Each emitted object owns a separate manager. RuntimeDyld
                // finalizes its RW pages to RX (code), R (constants), RW (data)
                // before lookup publishes an executable address; teardown frees
                // the mappings. No mutable/executable dual map or RWX allocator.
                auto layer = std::make_unique<orc::RTDyldObjectLinkingLayer>(session,
                    [](const ir::MemoryBuffer&) { return std::make_unique<ir::SectionMemoryManager>(); });
                layer->setOverrideObjectFlagsWithResponsibilityFlags(true);
                layer->setAutoClaimResponsibilityForObjectSymbols(true);
                return std::unique_ptr<orc::ObjectLayer>(std::move(layer));
            }).create();
        if (!result)
            throw std::runtime_error(ir::toString(result.takeError()));
        auto created = std::move(*result);
        orc::MangleAndInterner mangle(created->getExecutionSession(), created->getDataLayout());
        orc::SymbolMap symbols;
        // Optimizers/code generation may lower bulk stores to libc intrinsics.
        // Bind their actual host ABI too, without searching process exports.
        symbols[mangle("memcpy")] = orc::ExecutorSymbolDef::fromPtr(&std::memcpy, ir::JITSymbolFlags::Exported);
        symbols[mangle("memmove")] = orc::ExecutorSymbolDef::fromPtr(&std::memmove, ir::JITSymbolFlags::Exported);
        symbols[mangle("memset")] = orc::ExecutorSymbolDef::fromPtr(&std::memset, ir::JITSymbolFlags::Exported);
#define BIND_HELPER(name) symbols[mangle("RSP_" #name)] = orc::ExecutorSymbolDef::fromPtr(&RSP_##name, ir::JITSymbolFlags::Exported);
        BIND_HELPER(MFC0) BIND_HELPER(MTC0)
        BIND_HELPER(MFC2) BIND_HELPER(MTC2) BIND_HELPER(CFC2) BIND_HELPER(CTC2)
        VECTOR_HELPERS(BIND_HELPER)
        MEMORY_HELPERS(BIND_HELPER)
#undef BIND_HELPER
        if (auto error = created->getMainJITDylib().define(orc::absoluteSymbols(std::move(symbols))))
            throw std::runtime_error(ir::toString(std::move(error)));
        jit = std::move(created);
    }

    Entry lookup(unsigned start) {
        if (active[start])
            return active[start];
        const auto count = region_end(state.imem, start) - start;
        const auto hash = code_hash(state.imem + start, count);
        for (const auto& cached : cache[start]) {
            if (cached->hash == hash && cached->words.size() == count &&
                std::memcmp(cached->words.data(), state.imem + start, count * sizeof(std::uint32_t)) == 0)
                return active[start] = cached->entry;
        }
        initialize();
        auto region = std::make_unique<Region>();
        region->hash = hash;
        region->words.assign(state.imem + start, state.imem + start + count);
        auto context = std::make_unique<ir::LLVMContext>();
        const auto name = "isle_rsp_region_" + std::to_string(serial++);
        auto module = std::make_unique<ir::Module>(name, *context);
        module->setDataLayout(jit->getDataLayout());
        module->setTargetTriple(jit->getTargetTriple());
        RegionIR generator(*module, name, start, region->words);
        if (ir::verifyModule(*module))
            throw std::runtime_error("Invalid LLVM RSP region IR");
        optimize(*module);
        auto tracker = jit->getMainJITDylib().createResourceTracker();
        if (auto error = jit->addIRModule(tracker, orc::ThreadSafeModule(std::move(module), std::move(context)))) {
            ir::consumeError(tracker->remove());
            throw std::runtime_error(ir::toString(std::move(error)));
        }
        auto symbol = jit->lookup(name);
        if (!symbol) {
            ir::consumeError(tracker->remove());
            throw std::runtime_error(ir::toString(symbol.takeError()));
        }
        region->entry = symbol->toPtr<Entry>();
        // Publish only after ownership has entered the durable exact-code cache.
        // A failed allocation leaves no callable dangling cache entry.
        const auto entry = region->entry;
        try {
            cache[start].push_back(std::move(region));
        } catch (...) {
            ir::consumeError(tracker->remove());
            throw;
        }
        active[start] = entry;
        return entry;
    }
};

CPU::CPU() : impl_(std::make_unique<Impl>()) {}
CPU::~CPU() = default;
CPUState& CPU::get_state() noexcept { return impl_->state; }
void CPU::invalidate_imem() noexcept { impl_->state.dirty_blocks = ~0u; }

ReturnMode CPU::run() {
    auto& state = impl_->state;
    if (!state.imem || !state.dmem)
        return MODE_HOST_ERROR;
    for (;;) {
        if (state.dirty_blocks || impl_->active_imem != state.imem) {
            impl_->active.fill(nullptr);
            impl_->active_imem = state.imem;
            state.dirty_blocks = 0;
        }
        state.pc &= 0xffc;
        Entry entry;
        // All compilation, allocation, lookup and error propagation occur in
        // ordinary host frames, never below a generated frame or helper call.
        try {
            entry = impl_->lookup(state.pc / 4);
        } catch (...) {
            return MODE_HOST_ERROR;
        }
        const auto mode = static_cast<ReturnMode>(enter(entry, &state));
        if (mode == MODE_CONTINUE)
            continue;
        if (mode == MODE_BREAK) {
            auto* status = state.cp0.cr[CP0_REGISTER_SP_STATUS];
            if (!status || !state.cp0.irq)
                return MODE_HOST_ERROR;
            *status |= SP_STATUS_BROKE | SP_STATUS_HALT;
            if (*status & SP_STATUS_INTR_BREAK)
                *state.cp0.irq |= 1;
        }
        return mode;
    }
}

#undef MEMORY_HELPERS
#undef VECTOR_HELPERS
} // namespace RSP::LLVM
