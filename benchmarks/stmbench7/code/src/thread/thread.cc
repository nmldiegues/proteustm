#include "pthread_wrap.h"

#include "thread_sb7.h"
#include "../random.h"
#include "../common/tid.h"
#include "../common/memory.h"
#include "../tm/tm_ptr.h"
#include "../tm/tm_spec.h"

void sb7::global_thread_init(int number_threads) {
	global_init_random();
	init_tid_generator();
	global_init_mem();
	global_init_obj_log();
	global_init_tm(number_threads);
}

#include "rapl.h"

void sb7::thread_init(int threadId) {
        init_thread_tid();
	if (threadId != -1) {
		bindThread(threadId);
	}
	thread_init_random();
	init_thread_mem();
	thread_init_obj_log();
	thread_init_tm();
}

void sb7::thread_clean() {
	thread_clean_tm();
}
