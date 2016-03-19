#ifndef STM_WLPDSTM
#include <setjmp.h>
#endif

#include "tm_tx.h"
#include "tm_spec.h"
#include "tm_ptr.h"
#include "../common/memory.h"

void* sb7::run_tx(sb7::tx_fun fun, int ro_flag, sb7::tx_param param,
		sb7::tx_abort_fun abort_fun, sb7::tx_abort_param abort_param) {
	void *ret = fun(param);
	return ret;
}
