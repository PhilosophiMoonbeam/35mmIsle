/* Synthetic GNU lightning consumer: no ROM, SDK producer, or GAME source. */
#include <lightning.h>
#include <stdio.h>
#include <string.h>

static int run_pair(void)
{
    jit_state_t *states[2] = {NULL, NULL};
    int result = 0;
    int i;
    for (i = 0; i != 2; ++i) {
        jit_state_t *_jit = states[i] = jit_new_state();
        jit_pointer_t code;
        jit_word_t (*function)(void) = NULL;
        if (!_jit) { result = 1; break; }
        jit_prolog();
        jit_movi(JIT_R0, 42 + i);
        jit_retr(JIT_R0);
        jit_epilog();
        code = jit_emit();
        if (!code || sizeof(function) != sizeof(code)) { result = 2; break; }
        memcpy(&function, &code, sizeof(function));
        /* Clear compile state while keeping the emitted machine code alive. */
        jit_clear_state();
        if (function() != 42 + i) { result = 3; break; }
    }
    for (i = 1; i >= 0; --i) {
        jit_state_t *_jit = states[i];
        if (_jit) jit_destroy_state();
    }
    return result;
}

int main(int argc, char **argv)
{
    int result;
    (void)argc;
    init_jit(argv[0]);
    result = run_pair();
    if (!result) result = run_pair();
    finish_jit();
    if (result) fprintf(stderr, "synthetic JIT proof failed: %d\n", result);
    else puts("synthetic JIT: generated 42/43, overlapping and sequential states passed");
    return result;
}
