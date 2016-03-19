/*This works only in this macro-form when coupled with thread.* (it assumes some variables and definition to be in scope) */
#ifndef _RECTM
   #define _RECTM

   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <arpa/inet.h>

   #define SRV_ADDR "127.0.0.1"
   #define SRV_PORT 8888
   #define BUFF_SIZE 100
   #define NEW_WKLD "NEW_WKLD\n"
   #define FEEDBACK "FEEDBACK\n"

   //Status w.r.t. current workload
   #define TO_OPTIMIZE 0
   #define OPTIMIZING 1
   #define OPTIMIZED 2

   //These keywords shared with the server
   #define LINEAR_S "LINEAR"
   #define HALF_S "HALF"
   #define GIVEUP_S "GIVEUP"

   #define S_TO_INT_RETRY_POLICY(S,P) if (strcmp(S,LINEAR_S) == 0) *P=1; else if (strcmp(S,HALF_S) == 0) *P=2; else *P=0

   //Shared with thread.c . Eventually, a simple include will make this redundant
   #define TINYSTM 0
   #define NOREC 1
   #define TL2 2
   #define SWISSTM 3
   #define HTM 4
   #define HYBRIDNOREC 5
   #define HYBRIDTL2 6
   //Shared keywords with the server
   #define TINYSTM_S "tinystm"
   #define TL2_S "tl2"
   #define SWISSTM_S "swisstm"
   #define NOREC_S "norec"
   #define HTM_S "htm"
   #define HYBRIDNOREC_S "hybrid-norec-ptr"
   #define HYBRIDTL2_S "hybrid-tl2-ptr"

   //NB: no HTM for now, as it is not currently supported
   #define S_TO_INT_BACKEND(S,B) DR(printf("Converting %s to backend int\n",S));\
                                 if(strcmp(S,TINYSTM_S) == 0) *B = TINYSTM;\
                                 else if(strcmp(S,TL2_S) == 0) *B = TL2;\
                                 else if(strcmp(S,SWISSTM_S) == 0) *B = SWISSTM;\
                                 else if(strcmp(S,NOREC_S) == 0) *B = NOREC;\
                                 else if(strcmp(S,HTM_S) == 0) *B = HTM;\
                                 else if(strcmp(S,HYBRIDNOREC_S) == 0) *B = HYBRIDNOREC;\
                                 else *B = HYBRIDTL2_S


   short status = OPTIMIZED;
   char sendline[BUFF_SIZE];
   char recvline[BUFF_SIZE];

   #define DEBUG_RECTM
   #ifdef DEBUG_RECTM
      #define DR(s) s
   #else
      #define DR(s)
   #endif  //DEBUG_RECTM

   #define RANDOM_OVERRIDE

   /*TODO: Logic to signal new workload has to put status to TO_OPTIMIZE. Then, everything should run smoothly*/

   /*
     This is the logic to translate an incoming string to the config to deploy.
     We assume that the incoming string has a fixed format, so the burden of complying with this logic is on the server
     STM: backend-threads
     HTM&HYB: backend-threads-retry_policy-retry_budget
     The delimiter is a \n
     NB: as I don't know (yet) how threads, retry etc are encoded, I simply set the backend %4
   */


   #define PARSE_CONFIG(N,recvline) do{\
                                 char *instruction, *backend_s, *thread_s, *retry_policy_s, *retry_budget_s;\
                                 int retry_budget_i, retry_policy_i, backend_i, thread_i;\
                                 char *reply = strtok(strdup(recvline), "\n");\
                                 DR(printf("Reply was %s\n",recvline));\
                                 instruction = reply;\
                                 backend_s = strtok(NULL,"\n");\
                                 S_TO_INT_BACKEND(backend_s,&backend_i);\
                                 thread_s = strtok(NULL,"\n");\
                                 thread_i = atoi(thread_s);\
                                 retry_policy_s = strtok(NULL,"\n");\
                                 S_TO_INT_RETRY_POLICY(retry_policy_s,&retry_policy_i);\
                                 retry_budget_s = strtok(NULL,"\n");\
                                 retry_budget_i = atoi(retry_budget_s);\
                                 DR(printf("Tokenized to %s %d %d %d %d\n",instruction,backend_i,thread_i,retry_policy_i,retry_budget_i));\
                                 memset(recvline, 0, sizeof recvline);\
                                 N->backend = backend_i;\
                                 N->num_threads = thread_i;\
                                 N->htm_retry_policy = retry_policy_i;\
                                 N->htm_num_retries = retry_budget_i;\
                             }while(0)

   #define CONNECT_SOCKET(sockfd,servaddr)  DR(printf("Connecting socket on %s:%d\n",SRV_ADDR,SRV_PORT));\
                                 bzero(&servaddr, sizeof(servaddr));\
                                 servaddr.sin_family = AF_INET;\
                                 servaddr.sin_addr.s_addr = inet_addr(SRV_ADDR);\
                                 servaddr.sin_port = htons(SRV_PORT);\
                                 if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){\
                                    printf("Could not connect to server %s:%d\n",SRV_ADDR,SRV_PORT);\
                                    exit(-1);\
                                 }\
                                 DR(printf("Connection to server successfully established\n"));\

    #define OPEN_SOCKET(sockfd) DR(printf("Opening socket on %s:%d\n",SRV_ADDR,SRV_PORT));\
                                sockfd = socket(AF_INET, SOCK_STREAM, 0);\
                                if (sockfd <= 0) {\
                                   printf("Could not connect to server\n");\
                                   exit(-1);\
                                }\
                                DR(printf("Socket has been successfully opened\n"));\


   #ifdef KPI_TRACKING
   #include "workload.h"
   #define RESET_WL()  RESET_WORKLOAD_TRACKING_INFO()
   #else
   #define RESET_WL()
   #endif

   /*
   This could be a function, given that is not invoked so often. However, in its current form, it is so simple that we
   might want to save cpu cycles macro-ing this logic
   1) Connect to server socket
   2)
   */
   #define RECTM_INIT_CONFIG(N) do { \
                                 DR(printf("Going to request first configuration to deploy\n"));\
                                 struct sockaddr_in servaddr;\
                                 int sockfd;\
                                 OPEN_SOCKET(sockfd);\
                                 CONNECT_SOCKET(sockfd,servaddr);\
                                 char init[] = NEW_WKLD;\
                                 int i = 0;\
                                 memcpy(sendline, init, sizeof init);\
                                 int ret_wrt = sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));\
                                 if (ret_wrt == -1) {\
                                    printf("Write to server has failed\n");\
                                    exit(-1);\
                                 }\
                                 DR(printf("Successfully written %s to server\n", sendline));\
                                 DR(printf("Cleaning the buffer to receive configuration\n"));\
                                 memset(recvline, 0, sizeof recvline);\
                                 DR(printf("Now waiting for initial configuration from server\n"));\
                                 int n = recvfrom(sockfd, recvline, BUFF_SIZE, 0, NULL, NULL);\
                                 if (n <= 0) {\
                                    printf("Something's gone wrong. Exiting\n");\
                                    exit(-1);\
                                 }\
                                 DR(printf("%d bytes received for configuration %s\n", n, recvline));\
                                 PARSE_CONFIG(N,recvline);\
                                 DR(printf("Integer config %d\n",*N));\
                                 status = OPTIMIZING;\
                               } while (0)


   #define RECTM_EXPLORE_CONFIG(N,K) do { \
                                       DR(printf("Going to give feedback %f\n",K));\
                                       double kpi = K;\
                                       struct sockaddr_in servaddr;\
                                       int sockfd;\
                                       OPEN_SOCKET(sockfd);\
                                       CONNECT_SOCKET(sockfd,servaddr);\
                                       char feedback[] = FEEDBACK;\
                                       int ret_wrt = sendto(sockfd, feedback, strlen(feedback), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));\
                                       memset(sendline, 0, sizeof sendline);\
                                       sprintf(sendline, "%f\n", kpi);\
                                       DR(printf("Sent %s\n",sendline));\
                                       ret_wrt = sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));\
                                       DR(printf("Feedback sent, now waiting for instructions\n"));\
                                       memset(recvline, 0, sizeof recvline);\
                                       int n = recvfrom(sockfd, recvline, BUFF_SIZE, 0, NULL, NULL);\
                                       if (n <= 0) {\
                                          printf("Something's gone wrong. Exiting\n");\
                                          exit(-1);\
                                       }\
                                       if ( strstr(recvline, "END_WKLD") != NULL ){\
                                          DR(printf("Setting status to OPTIMIZED\n"));\
                                          status = OPTIMIZED;\
                                          RESET_WL();\
                                       }\
                                       PARSE_CONFIG(N,recvline);\
                                   } while (0)

   #define RECTM_NEW_CONFIG(N,K) if ( status == TO_OPTIMIZE) {RECTM_INIT_CONFIG(N);} else if ( status == OPTIMIZING ) {RECTM_EXPLORE_CONFIG(N,K);}



#endif  //_RECTM

