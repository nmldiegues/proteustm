#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <unistd.h>
#include <immintrin.h>
#include <rtmintrin.h>
#include <sys/time.h>
#include <signal.h>
#include "rapl.h"

#include "thread.h"

// Defined here in case we are not using GCC.
#ifndef GCC_RECTM
void reset_nesting_level() { statistics_array[thread_id].stats.nesting_level = 1; }
__attribute__ ((noreturn)) void _ITM_siglongjmp(int val, sigjmp_buf env) { siglongjmp(env, val); }
#endif


#define MAXIMUM_NUMBER_THREADS 128

__attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t statistics_array[MAXIMUM_NUMBER_THREADS];
__attribute__((aligned(CACHE_LINE_SIZE))) statistics_t last_statistics_array[MAXIMUM_NUMBER_THREADS];
struct timeval wndw_start;
struct timeval wndw_end;
struct timeval timer_start;
struct timeval timer_stop;

//Could we make this short?
int spawned_controller = 0, current_htm_retry_policy = 0, current_htm_retry_budget = 0;
int initialized_global = 0;
long total_threads = 0;
long stamp_total_threads = 0;
__attribute__((aligned(CACHE_LINE_SIZE))) unblockable_threads_t unblockable_threads;
__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t active_threads;
__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t htm_single_global_lock;
__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t fallback_in_use;
__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t exists_sw;

__thread unsigned int thread_id;
__thread int last_config_seen;
__thread norec_Thread* global_norec_thread;
__thread Thread* global_tl2_thread;
__thread unsigned short htm_tries;
__thread unsigned short read_only_htm;
__thread vwLock next_commit;

__attribute__((aligned(CACHE_LINE_SIZE))) padded_mutex_t reconfig_lock;

__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t thread_flags[MAXIMUM_NUMBER_THREADS];
__attribute__((aligned(CACHE_LINE_SIZE))) padded_cond_var_t thread_cond_vars[MAXIMUM_NUMBER_THREADS];
__attribute__((aligned(CACHE_LINE_SIZE))) padded_mutex_t thread_mutexes[MAXIMUM_NUMBER_THREADS];

# define TINYSTM 0
# define NOREC 1
# define TL2 2
# define SWISSTM 3
# define HTM 4
# define HYBRIDNOREC 5
# define HYBRIDTL2 6

//(For now) These must be consistent with rectm.h
# define GIVEUP 0
# define LINEAR 1
# define HALF 2

# define DEFAULT_HTM_RETRY_BUDGET 5
# define DEFAULT_HTM_RETRY_POLICY LINEAR

# ifndef STARTING_MODE
# define STARTING_MODE HTM
# endif
# ifndef RETRY_POLICY
# define RETRY_POLICY DEFAULT_HTM_RETRY_POLICY
# endif
# ifndef HTM_RETRIES
# define HTM_RETRIES DEFAULT_HTM_RETRY_BUDGET
# endif

//switching aux-variables
volatile int CURRENTMODE = STARTING_MODE;
int DEFAULT_CONFIGURATION = STARTING_MODE;

// Change to 1 to enable printfs surrounded by the D(ebug) macro
#if 0
#  define D(x) x
#else
#  define D(x)
#endif

#define TIMER_DIFF_SEC(start, stop) \
(((double)(stop.tv_sec)  + (double)(stop.tv_usec / 1000000.0)) - \
((double)(start.tv_sec) + (double)(start.tv_usec / 1000000.0)))

#define TIMER_DIFF_MSEC(start,stop) (stop.tv_sec - start.tv_sec) * 1000.0 + (stop.tv_usec - start.tv_usec) / 1000.0

/*Updates the last_statistics_array with the current stats values and returns a statistics struct with the delta*/
void update_last_window_statistics(statistics_t* diff){
    unsigned long temp;
    int i = 0;
    for (; i < total_threads; i++) {
        D(printf("[%d].commits = %lu\n",i,statistics_array[i].stats.commits));
        diff->commits += statistics_array[i].stats.commits - last_statistics_array[i].commits;
        last_statistics_array[i].commits = statistics_array[i].stats.commits;
        
        D(printf("[%d].aborts = %lu\n",i,statistics_array[i].stats.aborts));
        diff->aborts += statistics_array[i].stats.aborts - last_statistics_array[i].aborts;
        last_statistics_array[i].aborts = statistics_array[i].stats.aborts;
    }
}

/*Initialize the monitoring statistics: start the observation window and set commits/aborts to 0*/
void init_monitoring_stats(){
   gettimeofday(&wndw_start, NULL);\
   int i = 0;
    for (; i < total_threads; i++) {
        last_statistics_array[i].commits = 0;
        last_statistics_array[i].aborts = 0;
    }
}

#define USE_RECTM
#ifdef USE_RECTM
#include "rectm.h"

#ifdef KPI_TRACKING
#include "workload.h"
#define CONSUME_NEW_SAMPLE(S) CONSUME_NEW_SAMPLE_W(S)
#define OBTAIN_SMOOTH_KPI(S) OBTAIN_SMOOTH_KPI_W(S)
#else
#define CONSUME_NEW_SAMPLE(S)
#define OBTAIN_SMOOTH_KPI(S)
#endif //SMOOTH_KPI


#define KPI_THROUGHPUT  0
#define KPI_XACT_PER_JOULE  1
#ifndef TARGET_KPI
#define TARGET_KPI KPI_THROUGHPUT
#endif //TARGET_KPI
#define GET_KPI(KPI) if (TARGET_KPI == KPI_THROUGHPUT) KPI = throughput; else KPI = xact_per_joule

#define NEW_CONFIG(N) do{\
long int elapsedTime = 0;\
double throughput = 0, deltaEnergy = 0, delay = 0, xact_per_joule = 0, target_kpi = 0;\
statistics_t diff;\
diff.commits=0;\
diff.aborts=0;\
update_last_window_statistics(&diff);\
throughput = diff.commits;\
gettimeofday(&wndw_end, NULL);\
D(printf("START.sec %ld START.usec %ld END.sec %ld END.usec %ld\n",wndw_start.tv_sec,wndw_start.tv_usec,wndw_end.tv_sec,wndw_end.tv_usec));\
elapsedTime = TIMER_DIFF_MSEC(wndw_start,wndw_end);\
D(printf("Elapsed time since last reconfig %ld, total commits %lu\n",elapsedTime,diff.commits));\
wndw_start = wndw_end;\
throughput/=(double)elapsedTime;\
deltaEnergy = deltaEnergyIntel();\
xact_per_joule = (double)diff.commits / deltaEnergy;\
D(printf("Energy consumed in the last interval %f, commits %lu, corresponding to xact/joule of %f\n",deltaEnergy,diff.commits,xact_per_joule));\
GET_KPI(target_kpi);\
CONSUME_NEW_SAMPLE(target_kpi);\
OBTAIN_SMOOTH_KPI(target_kpi);\
/*D(printf("_T_KPI is %f\n",target_kpi));*/\
RECTM_NEW_CONFIG(N,target_kpi);\
}while(0)
#else
#define NEW_CONFIG(N) N->backend = rand() % 3; if (N->backend == 2) N->backend = 3; N->num_threads = rand() % 8 + 1;
#endif

#define NEW_TM(CONFIG,NEW_TM) NEW_TM = CONFIG->backend
#define NEW_THREADS(CONFIG,NEW_THREADS) NEW_THREADS = CONFIG->num_threads
#define NEW_HTM_RETRY(CONFIG,NEW_POLICY,NEW_BUDGET) NEW_POLICY = CONFIG->htm_retry_policy; NEW_BUDGET = CONFIG->htm_num_retries

__inline__ unsigned long long tick()
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#ifndef CONTROLLER_SLEEP
   #define CONTROLLER_SLEEP  5000000
#endif
void controller() {
#ifdef NO_ADAPTIVITY
    return;
#endif
    
    while (1) {
        usleep(CONTROLLER_SLEEP);
        /*Init new to old, so that if new_config does nothing, you know new == old*/
        int old = CURRENTMODE;
        int new_tm, new_number_of_activated_threads, new_htm_retry_policy, new_htm_retry_budget;
        tm_config_t *new_tm_config = malloc(sizeof (tm_config_t));
        if(new_tm_config == NULL){
            printf("Error while allocating new_tm_config\n");
            exit(-1);
        }
        //We fill the new_config with the current values, so that if no new config is suggested, we have the old values there
        new_tm_config->backend = old;
        new_tm_config->num_threads = active_threads.counter;
        new_tm_config->htm_retry_policy = current_htm_retry_policy;
        new_tm_config->htm_num_retries = current_htm_retry_budget;
        NEW_CONFIG(new_tm_config);
        NEW_TM(new_tm_config,new_tm);
        if (new_tm == HTM) new_tm = TINYSTM;
        NEW_THREADS(new_tm_config,new_number_of_activated_threads);
        NEW_HTM_RETRY(new_tm_config,new_htm_retry_policy, new_htm_retry_budget);
        D(printf("_R_ %d %d %d %d\n",new_tm_config->backend,new_tm_config->num_threads,new_tm_config->htm_retry_policy,new_tm_config->htm_num_retries));
        free(new_tm_config);
        D(printf("Old config: %d, new proposed config %d\n",old,new_tm));
        D(printf("Adapting number of threads from %d to %d\n", active_threads.counter,new_number_of_activated_threads));

#ifdef LATENCY_MEASUREMENT
        unsigned long start_ticks = tick();
#endif
        adapt_htm_retries(new_htm_retry_policy, new_htm_retry_budget);
		if(old != new_tm) {
            //change TM backend
		    D(printf("Old config: %d, new proposed config %d\n",old,new_tm));
		    adapt_threads(0);
			CURRENTMODE = new_tm;
		}
		adapt_threads(new_number_of_activated_threads);
#ifdef LATENCY_MEASUREMENT
		unsigned long end_ticks = tick();
		printf("Reconfiguration Latency: %d %d %d %d \t | %lu\n", new_tm, new_number_of_activated_threads,new_htm_retry_policy,new_htm_retry_budget, (end_ticks - start_ticks));
#endif

                fflush(stdout);
    }
}

void adapt_htm_retries(int new_htm_retry_policy, int new_htm_retry_budget){
    D(printf("Changing retry policy from %d to %d\n",current_htm_retry_policy,new_htm_retry_policy));
    current_htm_retry_policy = new_htm_retry_policy;
    D(printf("Changing retry budget from %d to %d\n",current_htm_retry_budget,new_htm_retry_budget));
    current_htm_retry_budget = new_htm_retry_budget;
}

void greentm_before_tx() {
    if(last_config_seen != CURRENTMODE) {
        reconfigure();
    }
	unsigned long val = START_EXEC_TXS(thread_id);
	if (IS_BLOCKED(val)) {
	    D(printf("%d ] is in STATE_BLOCK, obliging and stopping\n", thread_id));
	    NO_LONGER_EXEC_TXS(thread_id);
        pthread_mutex_lock(&thread_mutexes[thread_id].mutex);
	    // Ensure no other thread has awaken me in the time btw I check the flag and grab lock
	    if (IS_BLOCKED(thread_flags[thread_id].counter)) {
	        pthread_cond_wait(&thread_cond_vars[thread_id].cond_var, &thread_mutexes[thread_id].mutex);
	    }
	    START_EXEC_TXS(thread_id);
        pthread_mutex_unlock(&thread_mutexes[thread_id].mutex);
	    D(printf("%d ] was unblocked and is back to running\n", thread_id));
	    if(last_config_seen != CURRENTMODE) {
	        reconfigure();
	    }
	}
}

void reconfigure() {
	last_config_seen = CURRENTMODE;
	//every thread updates its fun ptrs to new TM backend
	switch(CURRENTMODE) {
	case TINYSTM:
		//switch to tinystm
	    D(printf("%d ] switching to tinystm\n",thread_id));
		abortFunPtr = &tinystm_abort;
		sharedReadU64FunPtr = &tinystm_sharedReadU64;
		sharedReadU32FunPtr = &tinystm_sharedReadU32;
		sharedReadU16FunPtr = &tinystm_sharedReadU16;
		sharedReadU8FunPtr = &tinystm_sharedReadU8;
		sharedReadFunPtr = &tinystm_sharedRead;
		sharedWriteU64FunPtr = &tinystm_sharedWriteU64;
		sharedWriteU32FunPtr = &tinystm_sharedWriteU32;
		sharedWriteU16FunPtr = &tinystm_sharedWriteU16;
		sharedWriteU8FunPtr = &tinystm_sharedWriteU8;
		sharedWriteFunPtr = &tinystm_sharedWrite;
		freeFunPtr = &tinystm_free;
		tmBeginFunPtr = &tinystm_begin;
		tmEndFunPtr = &tinystm_end;
		break;
	case NOREC:
		//switch to norec
		D(printf("%d ] switching to norec\n",thread_id));
		abortFunPtr = &norec_abort;
        sharedReadU64FunPtr = &norec_sharedReadU64;
        sharedReadU32FunPtr = &norec_sharedReadU32;
        sharedReadU16FunPtr = &norec_sharedReadU16;
        sharedReadU8FunPtr = &norec_sharedReadU8;
        sharedReadFunPtr = &norec_sharedRead;
        sharedWriteU64FunPtr = &norec_sharedWriteU64;
        sharedWriteU32FunPtr = &norec_sharedWriteU32;
        sharedWriteU16FunPtr = &norec_sharedWriteU16;
        sharedWriteU8FunPtr = &norec_sharedWriteU8;
        sharedWriteFunPtr = &norec_sharedWrite;
		freeFunPtr = &norec_free;
		tmBeginFunPtr = &norec_begin;
		tmEndFunPtr = &norec_end;
		break;
	case TL2:
		//switch to tl2
		D(printf("%d ] switching to tl2\n",thread_id));
		abortFunPtr = &tl2_abort;
        sharedReadU64FunPtr = &tl2_sharedReadU64;
        sharedReadU32FunPtr = &tl2_sharedReadU32;
        sharedReadU16FunPtr = &tl2_sharedReadU16;
        sharedReadU8FunPtr = &tl2_sharedReadU8;
        sharedReadFunPtr = &tl2_sharedRead;
        sharedWriteU64FunPtr = &tl2_sharedWriteU64;
        sharedWriteU32FunPtr = &tl2_sharedWriteU32;
        sharedWriteU16FunPtr = &tl2_sharedWriteU16;
        sharedWriteU8FunPtr = &tl2_sharedWriteU8;
        sharedWriteFunPtr = &tl2_sharedWrite;
		freeFunPtr = &tl2_free;
		tmBeginFunPtr = &tl2_begin;
		tmEndFunPtr = &tl2_end;
		break;
	case SWISSTM:
		//switch to swisstm
		D(printf("%d ] switching to swisstm\n",thread_id));
		abortFunPtr = &swisstm_abort;
        sharedReadU64FunPtr = &swisstm_sharedReadU64;
        sharedReadU32FunPtr = &swisstm_sharedReadU32;
        sharedReadU16FunPtr = &swisstm_sharedReadU16;
        sharedReadU8FunPtr = &swisstm_sharedReadU8;
        sharedReadFunPtr = &swisstm_sharedRead;
        sharedWriteU64FunPtr = &swisstm_sharedWriteU64;
        sharedWriteU32FunPtr = &swisstm_sharedWriteU32;
        sharedWriteU16FunPtr = &swisstm_sharedWriteU16;
        sharedWriteU8FunPtr = &swisstm_sharedWriteU8;
        sharedWriteFunPtr = &swisstm_sharedWrite;
		freeFunPtr = &swisstm_free;
		tmBeginFunPtr = &swisstm_begin;
		tmEndFunPtr = &swisstm_end;
		break;
    case HTM:
        //switch to htm
        D(printf("t%d switching to Intel RTM\n",thread_id));
        abortFunPtr = &htm_abort;
        sharedReadU64FunPtr = &htm_sharedReadU64;
        sharedReadU32FunPtr = &htm_sharedReadU32;
        sharedReadU16FunPtr = &htm_sharedReadU16;
        sharedReadU8FunPtr = &htm_sharedReadU8;
        sharedReadFunPtr = &htm_sharedRead;
        sharedWriteU64FunPtr = &htm_sharedWriteU64;
        sharedWriteU32FunPtr = &htm_sharedWriteU32;
        sharedWriteU16FunPtr = &htm_sharedWriteU16;
        sharedWriteU8FunPtr = &htm_sharedWriteU8;
        sharedWriteFunPtr = &htm_sharedWrite;
        freeFunPtr = &htm_free;
        tmBeginFunPtr = &htm_begin;
        tmEndFunPtr = &htm_end;
        break;
    case HYBRIDNOREC:
          //switch to HybridNorec
          D(printf("t%d switching to HybridNorec\n",thread_id));
          abortFunPtr = &htm_abort;
          sharedReadU64FunPtr = &htm_sharedReadU64;
          sharedReadU32FunPtr = &htm_sharedReadU32;
          sharedReadU16FunPtr = &htm_sharedReadU16;
          sharedReadU8FunPtr = &htm_sharedReadU8;
          sharedReadFunPtr = &htm_sharedRead;
          sharedWriteU64FunPtr = &hybridNorec_htm_sharedWriteU64;
          sharedWriteU32FunPtr = &hybridNorec_htm_sharedWriteU32;
          sharedWriteU16FunPtr = &hybridNorec_htm_sharedWriteU16;
          sharedWriteU8FunPtr = &hybridNorec_htm_sharedWriteU8;
          sharedWriteFunPtr = &hybridNorec_htm_sharedWrite;
          freeFunPtr = &htm_free;
          tmBeginFunPtr = &hybridNorec_begin;
          tmEndFunPtr = &hybridNorec_end;
          break;
    case HYBRIDTL2:
        //switch to HybridTl2
        D(printf("t%d switching to HybridTl2\n",thread_id));
        abortFunPtr = &htm_abort;
        sharedReadFunPtr = &htm_sharedRead;
        sharedWriteFunPtr = &hybridTl2_htm_sharedWrite;
        freeFunPtr = &htm_free;
        tmBeginFunPtr = &hybridTl2_begin;
        tmEndFunPtr = &hybridTl2_end;
        break;
	}
}

void adapt_threads(int target_number_threads) {
	pthread_mutex_lock(&reconfig_lock.mutex);
	D(printf("Adapting number threads to: %d\tNumber of active threads %d\n", target_number_threads, active_threads.counter));
	if(target_number_threads > active_threads.counter) {
		increment_threads(target_number_threads);
	} else if(target_number_threads < active_threads.counter){
		decrement_threads(target_number_threads);
	}
	pthread_mutex_unlock(&reconfig_lock.mutex);
}

void increment_threads(int target_number_threads) {    // Runs with reconfig_lock.mutex taken
    int i = 0;
    for (; i < total_threads && active_threads.counter < target_number_threads; i++) {
        pthread_mutex_lock(&thread_mutexes[i].mutex);
        if (IS_BLOCKED(thread_flags[i].counter)) {
            UNBLOCK(i);
            pthread_cond_signal(&thread_cond_vars[i].cond_var);
            D(printf("enable thread %d\n", i));
            active_threads.counter++;
        }
        pthread_mutex_unlock(&thread_mutexes[i].mutex);
    }

    int t = 0;
    while (unlikely(t < unblockable_threads.number_threads)) {
        int i = unblockable_threads.thread_ids[t];
        pthread_mutex_lock(&thread_mutexes[i].mutex);
        if (IS_BLOCKED(thread_flags[i].counter)) {
            UNBLOCK(i);
            pthread_cond_signal(&thread_cond_vars[i].cond_var);
            D(printf("enable thread %d\n", i));
            active_threads.counter++;
        }
        pthread_mutex_unlock(&thread_mutexes[i].mutex);
        t++;
    }
}

//return threads to wait before setting reconfiguration
void decrement_threads(int target_number_threads){    // Runs with reconfig_lock.mutex taken
    int i = total_threads - 1;
    for (; i >= 0 && active_threads.counter > target_number_threads; i--) {
        pthread_mutex_lock(&thread_mutexes[i].mutex);

        int t = 0;
        int cannotBlock = 0;
        while (unlikely(t < unblockable_threads.number_threads)) {
            if (unblockable_threads.thread_ids[t] == i) {
                cannotBlock = 1;
                break;
            }
            t++;
        }

        if (likely(cannotBlock == 0 && !IS_BLOCKED(thread_flags[i].counter))) {
            unsigned long v = BLOCK(i);
            D(printf("waiting for t%d to block\n",i));
            while (STILL_RUNNING(v)) {
                v = thread_flags[i].counter;
                _mm_pause();
            }
            D(printf("received \"I am blocked\" from t%d\n",i));
            active_threads.counter--;
        }
        pthread_mutex_unlock(&thread_mutexes[i].mutex);
    }

    if (target_number_threads == 0) {
        int t = 0;
        while (unlikely(t < unblockable_threads.number_threads)) {
            int i = unblockable_threads.thread_ids[t];
            pthread_mutex_lock(&thread_mutexes[i].mutex);
            if (!IS_BLOCKED(thread_flags[i].counter)) {
                unsigned long v = BLOCK(i);
                D(printf("waiting for t%d to block\n",i));
                while (STILL_RUNNING(v)) {
                    v = thread_flags[i].counter;
                    _mm_pause();
                }
                D(printf("received \"I am blocked\" from t%d\n",i));
                active_threads.counter--;
            }
            pthread_mutex_unlock(&thread_mutexes[i].mutex);
            t++;
        }
    }
}

void sig_term_handler(int signum, siginfo_t *info, void *ptr) {
    greentm_shutdown();
    printf("\nExited via signal handler and external kill\n");
    exit(13);
}

void greentm_startup(int number_of_threads) {

    if (!initialized_global) {
        initialized_global = 1;

        //make sure we print the stats in the end of the program no matter what
        static struct sigaction _sigact;
        memset(&_sigact, 0, sizeof(_sigact));
        _sigact.sa_sigaction = sig_term_handler;
        _sigact.sa_flags = SA_SIGINFO;
        sigaction(SIGTERM, &_sigact, NULL);

        //tinystm startup
        if (sizeof(long) != sizeof(void *)) {
            fprintf(stderr, "Error: unsupported long and pointer sizes\n");
            exit(1);
        }
        stm_init();
        mod_mem_init(0);
        if (getenv("STM_STATS") != NULL) {
            mod_stats_init();
        }

        //norec startup
        norec_TxOnce();

        //tl2 startup
        TxOnce();

        //swisstm startup
        wlpdstm_global_init();

        pthread_mutex_init(&reconfig_lock.mutex,NULL);

        // It is easier if we allocate an array so that it can be reallocated later on updates.
        unblockable_threads.thread_ids = (unsigned int*) malloc (sizeof(unsigned int));
        unblockable_threads.number_threads = 0;

    	int i;
    	for (i = 0; i < MAXIMUM_NUMBER_THREADS; i++){
             pthread_mutex_init(&thread_mutexes[i].mutex,NULL);
		     pthread_cond_init(&thread_cond_vars[i].cond_var,NULL);
    		INIT_TO_NOT_RUNNING(i);
    	}
    }

    CURRENTMODE = DEFAULT_CONFIGURATION;

    if (!spawned_controller && number_of_threads > 0) {
        /*(At least) for now, we initialize with a default/specified config, and we change back upon the first iteration
         of the control loop*/
        CURRENTMODE = DEFAULT_CONFIGURATION;
        active_threads.counter = 0;
        total_threads = number_of_threads;
        current_htm_retry_budget = HTM_RETRIES;
        current_htm_retry_policy = RETRY_POLICY;

        init_monitoring_stats();

        gettimeofday(&timer_start, NULL);
        //initialize controller thread
        spawned_controller = 1;
        pthread_t* tc = malloc(sizeof(pthread_t));
        pthread_create(tc, NULL, &controller, NULL);

        startEnergyIntel();
    }

    /* Hard-coded case to handle STMBench7 initializer thread
     This function is called twice in stmb7: once in init_data_holder  with threads=-1 and, after, in the start with the
     right amount of threads. So what we do is running the first time with tinystm and 1 thread  (to avoid some segfault
     happening with > 1 threads during this phase); upon second invocation (i.e., with the right amount of threads passed
     as argument) everything will setup normally and the benchmark will start with the specified #threads and backend
     */
    if (number_of_threads <= 0) {
        CURRENTMODE = TINYSTM;
    }
}

void greentm_shutdown() {
	/* We never shutdown. Either the benchmark will finish right after, or it will call startup again. So this is mostly useless.
    //tinystm shutdown
	if (getenv("STM_STATS") != NULL) {
		unsigned long u;
		if (stm_get_global_stats("global_nb_commits", &u) != 0)
			printf("#commits    : %lu\n", u);
		if (stm_get_global_stats("global_nb_aborts", &u) != 0)
			printf("#aborts     : %lu\n", u);
		if (stm_get_global_stats("global_max_retries", &u) != 0)
			printf("Max retries : %lu\n", u);
	}
	stm_exit();

	//norec shutdown
	norec_TxShutdown();

	//tl2 shutdown
	TxShutdown();

	//swisstm shutdown
	wlpdstm_global_shutdown();
	 */

    gettimeofday(&timer_stop,NULL);
    double wallClockTimeMSec = TIMER_DIFF_MSEC(timer_start,timer_stop);
    unsigned long commits = 0;
    unsigned long aborts = 0;
    int i = 0;
    for (; i < 128; i++) {
        if (statistics_array[i].stats.commits == 0) { break; }
        commits += statistics_array[i].stats.commits;
        aborts += statistics_array[i].stats.aborts;
        printf("[%d] C: %lu D: %lu\n",i,statistics_array[i].stats.commits,statistics_array[i].stats.aborts);
    }
    printf("Total commits: %lu\nTotal aborts: %lu\n", commits, aborts);
    printf("WCT (msec) %f\n",wallClockTimeMSec);
    double totalEnergy;
    endEnergyIntelRet(&totalEnergy);
    printf("_ENERGY_ %f\n",totalEnergy);
}

void greentm_thread_enter(int blockable) {
	/* init tinystm */
	stm_init_thread();
	/* init norec */
	global_norec_thread = norec_TxNewThread();
	norec_TxInitThread(global_norec_thread, thread_id);
	/* init tl2 */
	global_tl2_thread = TxNewThread();
	TxInitThread(global_tl2_thread, thread_id);
	/* init swisstm */
	wlpdstm_thread_init();

	reconfigure();

	pthread_mutex_lock(&reconfig_lock.mutex);
	pthread_mutex_lock(&thread_mutexes[thread_id].mutex);

	if (blockable == 0) {
	    // Deal with unblockable threads before they are "announced" to the controller.
	    unblockable_threads.number_threads++;
	    if (unblockable_threads.number_threads > 1) {
	        // In the initial case there is an array of 1 element that is unused, so we check for that here.
	        unblockable_threads.thread_ids = (unsigned int*) realloc(unblockable_threads.thread_ids, unblockable_threads.number_threads * sizeof(unsigned int));
	    }
	    unblockable_threads.thread_ids[unblockable_threads.number_threads - 1] = thread_id;
	}

	active_threads.counter++;
	if ((thread_id + 1) > total_threads) {
	    total_threads = thread_id + 1;
	    D(printf("New maximum number of total threads: %d\n", total_threads));
	}
	pthread_mutex_unlock(&thread_mutexes[thread_id].mutex);
	pthread_mutex_unlock(&reconfig_lock.mutex);
}

void greentm_thread_exit() {
	/* exit tinystm */
	stm_exit_thread();
	/* exit norec */
	norec_TxFreeThread(global_norec_thread);
	/* exit tl2 */
	TxFreeThread(global_tl2_thread);
	/* exit swisstm */
	wlpdstm_thread_shutdown();

    pthread_mutex_lock(&reconfig_lock.mutex);   // does not run concurrently with reconfiguration
    pthread_mutex_lock(&thread_mutexes[thread_id].mutex);
    active_threads.counter--;
    pthread_mutex_unlock(&thread_mutexes[thread_id].mutex);
    pthread_mutex_unlock(&reconfig_lock.mutex);   // does not run concurrently with reconfiguration
}

//tiny
int tinystm_begin(int ro, sigjmp_buf* buf, int setjmp_ret) {
    if (setjmp_ret == 0) { // this means we have not yet called longjmp on 'buf'
        stm_tx_attr_t _a = {};
        _a.read_only = ro;
        stm_start(_a, buf);
    }
    return 1;
}

void tinystm_end() {
    stm_commit();
}
void tinystm_free(void* ptr) {
    stm_free(ptr, sizeof(stm_word_t));
}

void tinystm_abort() {
    stm_abort(0);
}

uint64_t tinystm_sharedReadU64(uint64_t* addr) {
    return int_stm_load_u64(addr);
}

uint32_t tinystm_sharedReadU32(uint32_t* addr) {
    return int_stm_load_u32(addr);
}

uint16_t tinystm_sharedReadU16(uint16_t* addr) {
    return int_stm_load_u16(addr);
}

uint8_t tinystm_sharedReadU8(uint8_t* addr) {
    return int_stm_load_u8(addr);
}

intptr_t tinystm_sharedRead(intptr_t* addr) {
    return stm_load((volatile stm_word_t*) addr);
}

void tinystm_sharedWriteU64(uint64_t* addr, uint64_t val) {
    int_stm_store_u64(addr, val);
}

void tinystm_sharedWriteU32(uint32_t* addr, uint32_t val) {
    int_stm_store_u32(addr, val);
}

void tinystm_sharedWriteU16(uint16_t* addr, uint16_t val) {
    int_stm_store_u16(addr, val);
}

void tinystm_sharedWriteU8(uint8_t* addr, uint8_t val) {
    int_stm_store_u8(addr, val);
}

void tinystm_sharedWrite(intptr_t* addr, intptr_t val) {
    stm_store((volatile stm_word_t*) addr, (stm_word_t) val);
}


//NOrec
int norec_begin(int ro, sigjmp_buf* buf, int setjmp_ret) {
    if (setjmp_ret == 0) {
        norec_TxStart(global_norec_thread, buf);
    }
    return 1;
}
void norec_end() {
    norec_TxCommit(global_norec_thread);
}
void norec_free(void* ptr) {
}

void norec_abort() {
    norec_TxAbort(global_norec_thread);
}

uint64_t norec_sharedReadU64(uint64_t* addr) {
    return norec_TxLoad_U64(global_norec_thread, addr);
}

uint32_t norec_sharedReadU32(uint32_t* addr) {
    return norec_TxLoad_U32(global_norec_thread, addr);
}

uint16_t norec_sharedReadU16(uint16_t* addr) {
    return norec_TxLoad_U16(global_norec_thread, addr);
}

uint8_t norec_sharedReadU8(uint8_t* addr) {
    return norec_TxLoad_U8(global_norec_thread, addr);
}

intptr_t norec_sharedRead(intptr_t* addr) {
    return norec_TxLoad(global_norec_thread, addr);
}

void norec_sharedWriteU64(uint64_t* addr, uint64_t val) {
    norec_TxStore_U64(global_norec_thread, addr, val);
}

void norec_sharedWriteU32(uint32_t* addr, uint32_t val) {
    norec_TxStore_U32(global_norec_thread, addr, val);
}

void norec_sharedWriteU16(uint16_t* addr, uint16_t val) {
    norec_TxStore_U16(global_norec_thread, addr, val);
}

void norec_sharedWriteU8(uint8_t* addr, uint8_t val) {
    norec_TxStore_U8(global_norec_thread, addr, val);
}

void norec_sharedWrite(intptr_t* addr, intptr_t val) {
    norec_TxStore(global_norec_thread, addr, val);
}


//TL2
int tl2_begin(int ro, sigjmp_buf* buf, int setjmp_ret) {
    if (setjmp_ret == 0) {
        int read_only = ro;
        TxStart(global_tl2_thread, buf, &ro);
    }
    return 1;
}
void tl2_end() {
    TxCommit(global_tl2_thread);
}
void tl2_free(void* ptr) {
}

void tl2_abort() {
    TxAbort(global_tl2_thread);
}

uint64_t tl2_sharedReadU64(uint64_t* addr) {
    return TxLoad_U64(global_tl2_thread, addr);
}

uint32_t tl2_sharedReadU32(uint32_t* addr) {
    return TxLoad_U32(global_tl2_thread, addr);
}

uint16_t tl2_sharedReadU16(uint16_t* addr) {
    return TxLoad_U16(global_tl2_thread, addr);
}

uint8_t tl2_sharedReadU8(uint8_t* addr) {
    return TxLoad_U8(global_tl2_thread, addr);
}

intptr_t tl2_sharedRead(intptr_t* addr) {
    return TxLoad(global_tl2_thread, addr);
}

void tl2_sharedWriteU64(uint64_t* addr, uint64_t val) {
    TxStore_U64(global_tl2_thread, addr, val);
}

void tl2_sharedWriteU32(uint32_t* addr, uint32_t val) {
    TxStore_U32(global_tl2_thread, addr, val);
}

void tl2_sharedWriteU16(uint16_t* addr, uint16_t val) {
    TxStore_U16(global_tl2_thread, addr, val);
}

void tl2_sharedWriteU8(uint8_t* addr, uint8_t val) {
    TxStore_U8(global_tl2_thread, addr, val);
}

void tl2_sharedWrite(intptr_t* addr, intptr_t val) {
    TxStore(global_tl2_thread, addr, val);
}

//Swisstm
int swisstm_begin(int ro, sigjmp_buf* buf, int setjmp_ret) {
    if (setjmp_ret == 0) {
        wlpdstm_start_tx();
        wlpdstm_set_buf(buf);
    }
    return 1;
}
void swisstm_end() {
    wlpdstm_commit_tx();
}
void swisstm_free(void* ptr) {
}

void swisstm_abort() {
    wlpdstm_restart_tx();
}

uint64_t swisstm_sharedReadU64(uint64_t* addr) {
    return wlpdstm_read_64(addr);
}

uint32_t swisstm_sharedReadU32(uint32_t* addr) {
    return wlpdstm_read_32(addr);
}

uint16_t swisstm_sharedReadU16(uint16_t* addr) {
    return wlpdstm_read_16(addr);
}

uint8_t swisstm_sharedReadU8(uint8_t* addr) {
    return wlpdstm_read_8(addr);
}

intptr_t swisstm_sharedRead(intptr_t* addr) {
    return wlpdstm_read_word((Word*)addr);
}

void swisstm_sharedWriteU64(uint64_t* addr, uint64_t val) {
    wlpdstm_write_64(addr, val);
}

void swisstm_sharedWriteU32(uint32_t* addr, uint32_t val) {
    wlpdstm_write_32(addr, val);
}

void swisstm_sharedWriteU16(uint16_t* addr, uint16_t val) {
    wlpdstm_write_16(addr, val);
}

void swisstm_sharedWriteU8(uint8_t* addr, uint8_t val) {
    wlpdstm_write_8(addr, val);
}

void swisstm_sharedWrite(intptr_t* addr, intptr_t val) {
    wlpdstm_write_word((Word*)addr, (Word)val);
}

//htm Intel RTM
int htm_begin(int ro, sigjmp_buf* buf, int setjmp_ret) {
    htm_tries = current_htm_retry_budget;
    while (1) {
        while (*((volatile int*)(&htm_single_global_lock.counter))) {
            __asm__ ( "pause;");
        }
        unsigned int status = _xbegin();
        if (status == _XBEGIN_STARTED) {
            if (*((volatile int*)(&htm_single_global_lock.counter)) != 0) {
                _xabort(30);
            }
            break;
        }
        else if (status == _XABORT_CAPACITY) {
            spend_budget(&htm_tries);
        }
        else {
            htm_tries--;
        }
        if (htm_tries <= 0) {
            while (__sync_val_compare_and_swap(&htm_single_global_lock.counter, 0, 1) == 1) {
                __asm__ ("pause;");
            }
            break;
        }
    }
    return 0;
}
void htm_end() {
    if (htm_tries > 0) {
        _xend();
    } else {
        htm_single_global_lock.counter = 0;
    }
}
void htm_free(void* ptr) {
    free(ptr);
}
void htm_abort() {
    _xabort(0xab);
}
intptr_t htm_sharedRead(intptr_t* addr) {
    return *addr;
}

uint64_t htm_sharedReadU64(uint64_t* addr) {
    return *addr;
}
uint32_t htm_sharedReadU32(uint32_t* addr) {
    return *addr;
}

uint16_t htm_sharedReadU16(uint16_t* addr) {
    return *addr;
}

uint8_t htm_sharedReadU8(uint8_t* addr) {
    return *addr;
}

void htm_sharedWrite(intptr_t* addr, intptr_t val) {
    *addr = val;
}

void htm_sharedWriteU64(uint64_t* addr, uint64_t val) {
    *addr = val;
}

void htm_sharedWriteU32(uint32_t* addr, uint32_t val) {
    *addr = val;
}

void htm_sharedWriteU16(uint16_t* addr, uint16_t val) {
    *addr = val;
}

void htm_sharedWriteU8(uint8_t* addr, uint8_t val) {
    *addr = val;
}

//hybridNorec
int hybridNorec_begin(int ro, sigjmp_buf* buf, int setjmp_ret) {
    read_only_htm = 1;
    htm_tries = current_htm_retry_budget;
    while (1) {
        if (htm_tries > 0) {
            while (fallback_in_use.counter != 0) { __asm__ ( "pause;" ); }
            unsigned int status = _xbegin();
            if (status == _XBEGIN_STARTED) {
                if (fallback_in_use.counter != 0) { _xabort(0xab); }
                return 0;
            }
            else if (status == _XABORT_CAPACITY) {
                spend_budget(&htm_tries);
            }
            else {
                htm_tries--;
            }
        } else {
            abortFunPtr = &norec_abort;
            sharedReadFunPtr = &norec_sharedRead;
            sharedWriteFunPtr = &norec_sharedWrite;
            freeFunPtr = &norec_free;
            tmBeginFunPtr = &norec_begin;
            __sync_add_and_fetch(&exists_sw.counter,1);
            norec_begin(ro, buf, setjmp_ret);
            return 1;
        }
    }
}
void hybridNorec_end() {
    if (htm_tries > 0) {
        if (read_only_htm == 0 && exists_sw.counter != 0) {
            norec_HTM_INC_CLOCK();
        }
        _xend();
    } else {
        __sync_add_and_fetch(&fallback_in_use.counter,1);
        int ret = norec_TxCommitSTM(global_norec_thread);
        __sync_sub_and_fetch(&fallback_in_use.counter,1);
        if (ret == 0) {
            norec_TxAbort(global_norec_thread);
        }
        __sync_sub_and_fetch(&exists_sw.counter,1);
        abortFunPtr = &htm_abort;
        sharedReadFunPtr = &htm_sharedRead;
        sharedWriteFunPtr = &hybridNorec_htm_sharedWrite;
        freeFunPtr = &htm_free;
        tmBeginFunPtr = &hybridNorec_begin;
    }
}
void hybridNorec_htm_sharedWrite(intptr_t* addr, intptr_t val) {
    *addr = val;
    read_only_htm = 0;
}
void hybridNorec_htm_sharedWriteU64(uint64_t* addr, uint64_t val) {
	*addr = val;
	read_only_htm = 0;
}
void hybridNorec_htm_sharedWriteU32(uint32_t* addr, uint32_t val) {
	*addr = val;
	read_only_htm = 0;
}
void hybridNorec_htm_sharedWriteU16(uint16_t* addr, uint16_t val) {
	*addr = val;
	read_only_htm = 0;
}
void hybridNorec_htm_sharedWriteU8(uint8_t* addr, uint8_t val) {
	*addr = val;
	read_only_htm = 0;
}

//hybridTl2
int hybridTl2_begin(int ro, sigjmp_buf* buf, int setjmp_ret) {
    htm_tries = current_htm_retry_budget;
    while (1) {
        if (htm_tries > 0) {
            while (fallback_in_use.counter != 0) { __asm__ ( "pause;" ); }
            unsigned int status = _xbegin();
            if (status == _XBEGIN_STARTED) {
                if (fallback_in_use.counter != 0) { _xabort(0xab); }
                if (ro == 0) { next_commit = GVGenerateWV(global_tl2_thread, 0); }
                return 0;
            }
            else if (status == _XABORT_CAPACITY) {
                spend_budget(&htm_tries);
            }
            else {
                htm_tries--;
            }
        } else {
            abortFunPtr = &tl2_abort;
            sharedReadFunPtr = &tl2_sharedRead;
            sharedWriteFunPtr = &tl2_sharedWrite;
            freeFunPtr = &tl2_free;
            tmBeginFunPtr = &tl2_begin;
            tl2_begin(ro,buf,setjmp_ret);
            return 1;
        }
    }
}
void hybridTl2_end() {
    if (htm_tries > 0) {
        _xend();
    } else {
        __sync_add_and_fetch(&fallback_in_use.counter,1);
        int ret = TxCommitNoAbortSTM(global_tl2_thread);
        __sync_sub_and_fetch(&fallback_in_use.counter,1);
        if (ret == 0) {
            TxAbort(global_tl2_thread);
        }
        AfterCommit(global_tl2_thread);
        abortFunPtr = &htm_abort;
        sharedReadFunPtr = &htm_sharedRead;
        sharedWriteFunPtr = &hybridTl2_htm_sharedWrite;
        freeFunPtr = &htm_free;
        tmBeginFunPtr = &hybridTl2_begin;
    }
}
void hybridTl2_htm_sharedWrite(intptr_t* addr, intptr_t val) {
    *addr = val;
    TxStoreHTM(global_tl2_thread, addr, val, next_commit);
}

//Note that actually the retry policy can change while being here, but it is not a problem, as the last "else" will cause a simple linear decrement
void spend_budget(short* tries) {
    if(current_htm_retry_policy == GIVEUP)
        (*tries)=0;
    else if (current_htm_retry_policy == HALF)
        (*tries)=(*tries)/2;
    else (*tries)=--(*tries);
}

__thread void (*freeFunPtr)(void* ptr);
__thread void (*abortFunPtr)();
__thread uint64_t (*sharedReadU64FunPtr)(uint64_t* addr);
__thread uint32_t (*sharedReadU32FunPtr)(uint32_t* addr);
__thread uint16_t (*sharedReadU16FunPtr)(uint16_t* addr);
__thread uint8_t (*sharedReadU8FunPtr)(uint8_t* addr);
__thread intptr_t (*sharedReadFunPtr)(intptr_t* addr);
__thread void (*sharedWriteU64FunPtr)(uint64_t* addr, uint64_t val);
__thread void (*sharedWriteU32FunPtr)(uint32_t* addr, uint32_t val);
__thread void (*sharedWriteU16FunPtr)(uint16_t* addr, uint16_t val);
__thread void (*sharedWriteU8FunPtr)(uint8_t* addr, uint8_t val);
__thread void (*sharedWriteFunPtr)(intptr_t* addr, intptr_t val);
__thread int (*tmBeginFunPtr)(int ro, sigjmp_buf* buf, int setjmp_ret);
__thread void (*tmEndFunPtr)();


#include <assert.h>
#include <stdlib.h>
#include <sched.h>


#ifndef TYPES_H
#define TYPES_H 1

typedef unsigned long ulong_t;

enum {
    FALSE = 0,
    TRUE  = 1
};

typedef long bool_t;

#endif

static THREAD_LOCAL_T    global_threadId;
static THREAD_BARRIER_T* global_barrierPtr      = NULL;
static long*             global_threadIds       = NULL;
static THREAD_ATTR_T     global_threadAttr;
static THREAD_T*         global_threads         = NULL;
static void            (*global_funcPtr)(void*) = NULL;
static void*             global_argPtr          = NULL;
static volatile bool_t   global_doShutdown      = FALSE;

#ifdef GCC_RECTM
  #include "gcc-abi/libitm.h"
#endif

static void threadWait (void* argPtr)
{
    long threadId = *(long*)argPtr;

    THREAD_LOCAL_SET(global_threadId, (long)threadId);
    
    bindThread(threadId);
    
    thread_id = threadId;
    
    while (1) {

    #ifdef GCC_RECTM
        _ITM_initializeThread();
    #endif

        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for start parallel */
        if (global_doShutdown) {
            break;
        }
        global_funcPtr(global_argPtr);

    #ifdef GCC_RECTM
        _ITM_finalizeThread();
    #endif

        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for end parallel */
        if (threadId == 0) {
            endEnergy();
            break;
        }
    }
}

void thread_startup (long numThread)
{
	long i;

	stamp_total_threads = numThread;
	global_doShutdown = FALSE;

	/* Set up barrier */
	assert(global_barrierPtr == NULL);
	global_barrierPtr = THREAD_BARRIER_ALLOC(numThread);
	assert(global_barrierPtr);
	THREAD_BARRIER_INIT(global_barrierPtr, numThread);

	/* Set up ids */
	THREAD_LOCAL_INIT(global_threadId);
	assert(global_threadIds == NULL);
	global_threadIds = (long*)malloc(numThread * sizeof(long));
	assert(global_threadIds);
	for (i = 0; i < numThread; i++) {
		global_threadIds[i] = i;
	}

	/* Set up thread list */
	assert(global_threads == NULL);
	global_threads = (THREAD_T*)malloc(numThread * sizeof(THREAD_T));
	assert(global_threads);

	startEnergy();

	htm_single_global_lock.counter = 0;
	/* Set up pool */
	THREAD_ATTR_INIT(global_threadAttr);
	for (i = 1; i < numThread; i++) {
		THREAD_CREATE(global_threads[i],
				global_threadAttr,
				&threadWait,
				&global_threadIds[i]);
	}

	/*
	 * Wait for primary thread to call thread_start
	 */
}


void thread_start (void (*funcPtr)(void*), void* argPtr)
{
    global_funcPtr = funcPtr;
    global_argPtr = argPtr;
    
    long threadId = 0; /* primary */
    threadWait((void*)&threadId);
}


void thread_shutdown ()
{
	/* Make secondary threads exit wait() */
	global_doShutdown = TRUE;
	THREAD_BARRIER(global_barrierPtr, 0);

	long numThread = stamp_total_threads;

	long i;
	for (i = 1; i < numThread; i++) {
		THREAD_JOIN(global_threads[i]);
	}

	THREAD_BARRIER_FREE(global_barrierPtr);
	global_barrierPtr = NULL;

	free(global_threadIds);
	global_threadIds = NULL;

	free(global_threads);
	global_threads = NULL;

    stamp_total_threads = 1;
}

barrier_t *barrier_alloc() {
    return (barrier_t *)malloc(sizeof(barrier_t));
}

void barrier_free(barrier_t *b) {
    free(b);
}

void barrier_init(barrier_t *b, int n) {
    pthread_cond_init(&b->complete, NULL);
    pthread_mutex_init(&b->mutex, NULL);
    b->count = n;
    b->crossing = 0;
}

void barrier_cross(barrier_t *b, int reset_count) {
    pthread_mutex_lock(&b->mutex);
    /* One more thread through */
    b->crossing++;
    /* If not all here, wait */
    if (b->crossing < b->count) {
        pthread_cond_wait(&b->complete, &b->mutex);
    } else {
        /* Reset for next time */
        if (reset_count) {
            b->crossing = 0;
        }
        pthread_cond_broadcast(&b->complete);
    }
    pthread_mutex_unlock(&b->mutex);
}

void thread_barrier_wait()
{
    long threadId = thread_getId();
    THREAD_BARRIER(global_barrierPtr, threadId);
}


long thread_getId()
{
    return (long)THREAD_LOCAL_GET(global_threadId);
}


long thread_getNumThread()
{
    return stamp_total_threads;
}
