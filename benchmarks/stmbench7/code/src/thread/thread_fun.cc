#include <cmath>
#include <setjmp.h>

#include "pthread_wrap.h"
#include "../tm/tm_spec.h"
#include "../common/time.h"
#include "../common/memory.h"
#include "../tm/tm_tx.h"
#include "thread_sb7.h"
#include "thread_fun.h"

#include "../parameters.h"
#include "../data_holder.h"
#include "../sb7_exception.h"
#include "../helpers.h"

#include "../workloads.h"
#include "../operations/operations.h"


#ifdef CHANGEWORKLOAD
   #include <unistd.h>
#endif

namespace sb7 {
static void *init_single_tx(void *data) {
	DataHolder *dataHolder = (DataHolder *)data;
	dataHolder->init();
	return NULL;
}
}

void *sb7::init_data_holder(void *data) {
	// initialize thread local data
	global_thread_init(-1);
	thread_init(-1);

	int mode = 0;
	TM_BEGIN(0,mode);
	if(parameters.shouldInitSingleTx()) {
		run_tx(init_single_tx, 0, data);
	} else {
		DataHolder *dataHolder = (DataHolder *)data;
		dataHolder->initTx();
	}
	TM_END();

	// finish up this thread
	thread_clean();
	reset_thread_tid();
	// just return something
	return NULL;
}

void *sb7::worker_thread(void *data) {
	WorkerThreadData *wtdata = (WorkerThreadData *)data;
	thread_init(wtdata->threadId);
   #ifdef CHANGEWORKLOAD
	   wtdata->workload_index = 0;
	   std::cout << "thread " << wtdata->threadId << " started\n" << std::endl;
   #endif
	bool hintRo = parameters.shouldHintRo();
	
	int opind;
	
	while(!wtdata->stopped) {


#ifdef CHANGEWORKLOAD
		if(workloads.getIndex() != wtdata->workload_index) { \
			wtdata->workload_index = workloads.getIndex(); \
			wtdata->operations = new Operations(&(wtdata->dataHolder)); \
			std::cout << "The workload has changed to: " << workloads.getIndex() << std::endl; \
		}
#endif

#ifdef OPERATIONPROB
			opind = wtdata->operations->getRandOp();
#else
			opind = getOperationRndInd();
#endif

		const Operation *op = wtdata->operations->getOperations()[opind];
	
		// get start time
		long start_time = get_time_ms();

		int ro_flag = (hintRo && op->isReadOnly());
		int mode = 0;
		wtdata->have_restarted = 0;
		wtdata->have_failed = 0;
		TM_BEGIN_EXT(0,mode,ro_flag);  // TODO: deal with Hybrid paths for opt backends; also if we need an Op Id, it should be possible to get it from *op
		if (wtdata->have_failed == 0) {
			if (wtdata->have_restarted) {
				mem_tx_abort();
				obj_log_tx_abort();
			} else {
				wtdata->have_restarted = 1;
			}
			mem_tx_start();
			try {
				op->run();
			} catch (Sb7Exception) {
				// We consider application-failed transactions to be successful transactions
				wtdata->have_failed = 1;
				FAST_PATH_RESTART();
			}
		} else {
			mem_tx_abort();
			obj_log_tx_abort();
			mem_tx_start();
		}
		TM_END();

		mem_tx_commit();
		obj_log_tx_commit();

		// get end time
		long end_time = get_time_ms();
		//Especially with long traversals, we can have a xact finishing way after the end of the benchmark
		//We do not want to count them in the throughput (which is computed dividing by the time parameter passed as input
		//and not the actual elapsed wallclock time (which gets erroneously "stretched" by this "late" xact)
		if (!wtdata->stopped) {
			wtdata->successful_ops[opind]++;
			long ttc = end_time - start_time;

			if(ttc <= wtdata->max_low_ttc) {
				wtdata->operations_ttc[opind][ttc]++;
			} else {
				double logHighTtc = (::log(ttc) - wtdata->max_low_ttc_log) /
						wtdata->high_ttc_log_base;
				int intLogHighTtc =
						MIN((int)logHighTtc, wtdata->high_ttc_entries - 1);
				wtdata->operations_high_ttc_log[opind][intLogHighTtc]++;
			}
		}
	}
	thread_clean();
	// just return something
	return NULL;
}

//not being used now
int sb7::WorkerThreadData::getOperationRndInd() const {
	double oprnd = get_random()->nextDouble();
	const std::vector<double> &opRat = operations->getOperationCdf();
	int opind = 0;

	while(opRat[opind] < oprnd) {
		opind++;
	}

	return opind;
}

#ifdef CHANGEWORKLOAD
#ifndef WORKLOAD_PHASE_DURATION
#define WORKLOAD_PHASE_DURATION 60000
#endif
void *sb7::change_workload(void *data) {
	WorkerThreadData *wtdata = (WorkerThreadData *)data;

	while(!wtdata->stopped) {
		sleep(WORKLOAD_PHASE_DURATION);

		Workload* workload = workloads.getWorkload();

		std::cout << "Workload is changing..." << std::endl;
		workload->info();
		parameters.setReadOnlyOperationsRatio(workload->getRo());
		parameters.setWriteRoot(workload->getMr());
		parameters.setStructureModificationEnabled(workload->getSm());
		parameters.setLongTraversalsEnabled(workload->getLt());
	}
	std::cout << "stopped" << std::endl;
}
#endif
