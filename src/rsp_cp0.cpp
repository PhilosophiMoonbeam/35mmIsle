// Original private host-service boundary, not the upstream plugin/SDK CP0 body.
#include "state.hpp"

extern "C" int RSP_MFC0(RSP::CPUState* state, unsigned rt, unsigned rd) {
    if (rd >= 16 || rt >= 32 || !state->cp0.cr[rd]) {
        return RSP::MODE_HOST_ERROR;
    }
    const auto value = *state->cp0.cr[rd];
    if (rt != 0) {
        state->sr[rt] = value;
    }
    if (rd == RSP::CP0_REGISTER_SP_SEMAPHORE) {
        *state->cp0.cr[rd] = 1;
    }
    return RSP::MODE_CONTINUE;
}

extern "C" int RSP_MTC0(RSP::CPUState* state, unsigned rd, unsigned rt) {
    if (rd >= 16 || rt >= 32) {
        return RSP::MODE_HOST_ERROR;
    }
    // Return through the generated frame's normal epilogue before any DMA,
    // packet copy, callback, allocation or invalidation can run. This includes
    // DPC_END: the producer cannot overwrite even an incomplete packet first.
    state->service_register = rd;
    state->service_value = rt == 0 ? 0 : state->sr[rt];
    return RSP::MODE_HOST_SERVICE;
}
