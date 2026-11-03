// SPDX-License-Identifier: MIT
// Copyright (c) 2026 35mmIsle contributors
#ifndef ISLE_RSP_LLVM_HPP
#define ISLE_RSP_LLVM_HPP

#include "state.hpp"
#include <memory>

namespace RSP::LLVM {

// LLVM and executable-memory ownership stay private to the native backend.
class CPU {
public:
    CPU();
    ~CPU();
    CPU(const CPU&) = delete;
    CPU& operator=(const CPU&) = delete;
    CPU(CPU&&) = delete;
    CPU& operator=(CPU&&) = delete;

    CPUState& get_state() noexcept;
    void invalidate_imem() noexcept;
    ReturnMode run();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace RSP::LLVM
#endif
