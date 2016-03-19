/**
 * Mask differences related to long_jmp functions on different platforms.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_JMP_H_
#define WLPDSTM_JMP_H_

#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void reset_nesting_level();
extern void _ITM_siglongjmp(int val, sigjmp_buf env) __attribute__ ((noreturn));

#define LONG_JMP_BUF jmp_buf
#define LONG_JMP_FIRST_FLAG 0
#define LONG_JMP_RESTART_FLAG 1
#define LONG_JMP_ABORT_FLAG 2

static void RestartJumpInC(sigjmp_buf env, int val) {
    reset_nesting_level();
    _ITM_siglongjmp(val, env);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WLPDSTM_JMP_H_ */

