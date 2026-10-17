#ifndef ISLE_OWNED_COMMAND_FIXTURE_H
#define ISLE_OWNED_COMMAND_FIXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Return zero on success, otherwise the first failed C fixture source line. */
int owned_command_admission_regression(void);
int owned_command_completion_regression(void);
int owned_command_append_regression(void);
int owned_command_delete_regression(void);
int owned_command_token_regression(void);
int owned_command_cancel_regression(void);
int owned_command_fault_regression(void);
int owned_command_scope_regression(void);
int owned_command_legacy_wait_regression(void);
int owned_command_target_retirement_regression(void);
int owned_command_caller_join_regression(void);
int owned_command_legacy_fault_regression(void);

#ifdef __cplusplus
}
#endif

#endif
