/*This works only in this macro-form when coupled with rectm.h (it assumes some variables and definition to be in scope) */
#ifndef _WORKLOAD
#define _WORKLOAD

//# of kpi samples that are kept
#define HORIZON 10
//Smoothing parameter for EMWA
#ifndef SMOOTHING
   #define SMOOTHING 0.9  // give more weight to older values
#endif
//Consecutive # of anomalies to be detected before triggering the change workload alarm
#ifndef CONSECUTIVE_THRESHOLD
   #define CONSECUTIVE_THRESHOLD 3
#endif //CONSECUTIVE_THRESHOLD
//Percentage threshold to detect an anomaly/change
#ifndef ANOMALY_THRESHOLD
   #define ANOMALY_THRESHOLD 0.15
#endif
//Percentage of HORIZON we have to wait after a reconfiguration before considering a new one
#define COOLDOWN_WINDOW 2
#define STARTUP_WINDOW 2


static double kpi_history[HORIZON];
static bool anomaly_detected_history[HORIZON];

static unsigned int epoch = 0;
static unsigned int monothonic_epoch = 0;
static double current_smoothed_kpi=0;
static double previous_smoothed_kpi = 0;
static unsigned short consecutive_anomalies = 0;
static double diff_acum = 0.0;
static double previous_sample = 0.0;

#define DEBUG_KPI_TRACK 1
#ifdef DEBUG_KPI_TRACK
#define  DTK(x) x
#else
#define DTK(x)
#endif

//Not optimized: we could maintain a circular array with another int that marks the current index in the array, so as
//To avoid the linear shift every time a sample is added
//This macro fills the reservoir of last measurements, performs smoothing and increases the epoch number
#define CONSUME_NEW_SAMPLE_W(S)  if(epoch<HORIZON) {kpi_history[epoch] = S;}\
                                 else {\
                                     int i; \
                                    for(i=0;i<HORIZON-1;i++)\
                                       kpi_history[i] = kpi_history[i+1];\
                                 }\
                                 kpi_history[HORIZON-1] = S;\
                                 previous_smoothed_kpi = current_smoothed_kpi;\
                                 /*Do the smoothing in any case*/\
                                 GATHER_CUMULATIVE_CHANGES(previous_sample,S); \
                                 EMWA(current_smoothed_kpi,S,epoch);\
                                 DTK(printf("_T_KPI (%d, %d) Smoothed kpi is %f Unsmoothed is %f\n",monothonic_epoch,epoch,target_kpi,S));\
                                 WARMUP();\
                                 if(!(MASK_NEW_SAMPLE())){\
                                    DTK(printf("_T_WKLD (%d, %d) sample *not* masked",monothonic_epoch,epoch));\
                                    PRINT_CURRENT_CUSUM(previous_smoothed_kpi); \
                                    if(DETECT_CHANGE_CUSUM(previous_smoothed_kpi)) {\
                                       status = TO_OPTIMIZE;\
                                       DTK(printf("_T_WKLD (%d, %d) Triggering workload change\n",monothonic_epoch,epoch));\
                                    }\
                                 }\
                                 else{\
                                    DTK(printf("_T_WKLD (%d, %d) sample masked. Status is %d and cooldown is %d\n",monothonic_epoch,epoch,status,COOLDOWN_WINDOW));\
                                 }\
                                 previous_sample = S; \
                                 epoch++; monothonic_epoch++;

//This macro performs smoothing according to exponential moving weighted average
//NB: this averages everything from the beginning, NOT only measurements in the horizon
#define EMWA(CURRENT_MEAN,NEW_SAMPLE,CURRENT_EPOCH) if(CURRENT_EPOCH == 0) {CURRENT_MEAN = kpi_history[0];}\
                                                    else{CURRENT_MEAN = CURRENT_MEAN * SMOOTHING + NEW_SAMPLE * (1.0 - SMOOTHING);}\


#define OBTAIN_SMOOTH_KPI_W(S)  S=current_smoothed_kpi

//NB: the average value is always updated: if the change happens at epoch T we detect that KPI(T) != KPI (T-1)
//But at T+1 , we should have that KPI(T+1) == KPI(T), so we might never actually trigger the new workload!
//This might be solved by properly tuning the SMOOTHING parameter for the EMWA, but I'm not entirely sure of this yet
//In particular, if we give "good" relevance to old samples, we might have that the smoothed average value slowly
//Evolves towards the "new real" average value, giving us time to notice that there is a change that is happening
//As an alternative, when we see that there is an anomaly, we could FIX the mean value AM against which we noticed the anomaly
//If we have that the (moving) average is considered X times consecutively as anomalous with respect to the FIXED AM
//Then we might trigger the change in workload

/*
TODO: kpi can change in positive or negative direction. To increase the consecutive anomalies, we should check that the
TODO: change is happening consecutively in the same direction, in order to avoid considering oscillations as changing workloads
TODO: this is not currently done, as we are using simple variable workload functions and rather wide monitoring intervals
*/
#define DETECT_CHANGE_WITH_ADAPTIVE_THRESHOLD(CURRENT_SAMPLE, CURRENT_MEAN) if(CURRENT_SAMPLE >= (1.0 + ANOMALY_THRESHOLD) * CURRENT_MEAN ||\
                                                                              CURRENT_SAMPLE <= (1.0 - ANOMALY_THRESHOLD) * CURRENT_MEAN){\
                                                                              DTK(printf("_T_WKLD (%d, %d) Workload anomaly detected with current sample being %f and current mean being %f and total threshold being (%f, %f) \n",monothonic_epoch,epoch,CURRENT_SAMPLE,CURRENT_MEAN,(1.0 + ANOMALY_THRESHOLD) * CURRENT_MEAN,(1.0 - ANOMALY_THRESHOLD) * CURRENT_MEAN));\
                                                                              consecutive_anomalies++;\
                                                                              }\
                                                                            else {\
                                                                              DTK(printf("_T_WKLD (%d, %d) Workload anomaly *NOT* detected with current sample being %f and current mean being %f and total threshold being (%f, %f) \n",monothonic_epoch,epoch,CURRENT_SAMPLE,CURRENT_MEAN,(1.0 + ANOMALY_THRESHOLD) * CURRENT_MEAN,(1.0 - ANOMALY_THRESHOLD) * CURRENT_MEAN));\
                                                                              consecutive_anomalies=0;\
                                                                            }

#define GATHER_CUMULATIVE_CHANGES(PREVIOUS_SAMPLE,CURRENT_SAMPLE) diff_acum += CURRENT_SAMPLE - PREVIOUS_SAMPLE; DTK(printf("\tAcum: %f\tdelta: %f\n", diff_acum, CURRENT_SAMPLE - PREVIOUS_SAMPLE));

#define PRINT_CURRENT_CUSUM(PREVIOUS_MEAN) DTK(printf("\tAcum: %f\t must be within: [ %f, %f ]\n", diff_acum, (-1 * ANOMALY_THRESHOLD * PREVIOUS_MEAN), (ANOMALY_THRESHOLD * PREVIOUS_MEAN)));
#define DETECT_CHANGE_CUSUM(PREVIOUS_MEAN) (diff_acum >= ANOMALY_THRESHOLD * PREVIOUS_MEAN) || (diff_acum <= (-1 * ANOMALY_THRESHOLD * PREVIOUS_MEAN))

//Evaluates to true if the number of consecutive anomalies has reached the  threshold
#define HAS_WORKLOAD_CHANGED() consecutive_anomalies >= CONSECUTIVE_THRESHOLD

#define WARMUP()  if(monothonic_epoch == STARTUP_WINDOW && status == OPTIMIZED) status = TO_OPTIMIZE; else if (monothonic_epoch > STARTUP_WINDOW && epoch == (COOLDOWN_WINDOW - 1)) diff_acum = 0.0;

#define IS_WARMING_UP() (status == OPTIMIZED && monothonic_epoch < STARTUP_WINDOW)

//Mask the anomaly detection and new workload trigger if we are actually optimizing or cooling down
#define MASK_NEW_SAMPLE() status == OPTIMIZING || epoch < COOLDOWN_WINDOW || IS_WARMING_UP()

//Reset everything. NB: since we are not considering entries > epoch if epoch < HORIZON, we do not need to reset the array of past measurements
#define RESET_WORKLOAD_TRACKING_INFO() DTK(printf("Resetting workload info\n")); epoch = 0; consecutive_anomalies = 0; current_smoothed_kpi = 0; previous_smoothed_kpi = 0; diff_acum = 0.0;
//TODO: put a global epoch, and an epoch after last reset


#endif  //_WORKLOAD


//NB: CUSUM solutions, as far as I see, at least the simple ones, do not account for heteroskedasticity
//This is a problem, because if the response time/throughput varies, say, one order of magnitude, we cannot still think
//That the variance will be the same as before!
