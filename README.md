ProteusTM: An Autonomic Transactional Memory for GCC
==========

There are many designs and implementations of Transactional Memory, and that is not a coincidence: different ones perform best on different workloads.

One cannot expect the developer to be aware of such concerns, as it is even contradictory with the simplicity advocated by the use of the transactional abstraction.

With ProteusTM, the developer writes transactions (using the C++ construct), and the underlying implementation (inside the GCC) uses Recommendation Systems techniques to automatically change Transactional Memory algorithms, parallelism degree, and others.

When using this work, please cite accordingly: "ProteusTM: Abstraction Meets Performance in Transactional Memory", Diego Didona, Nuno Diegues, Rachid Guerraoui, Anne-Marie Kermarrec, Ricardo Neves and Paolo Romano, in Proceedings of the 21st International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS), 2016.

This work was supported by national funds through Fundação para a Ciência e Tecnologia (FCT) with reference UID/CEC/50021/2013 and by the GreenTM project (EXPL/EEI-ESS/0361/2013).


## Framework with a variety of TM implementations and benchmarks ##


**Requirements:**

* Install: tcmalloc (needed for all backends), libevent (needed for memcached) and C++ boost library (needed for tpc-c)

* Compile: each stm (within "stms") and the rapl library (within "rapl-power") with the usual "make clean; make"


**Benchmarks:**

* STAMP suite (genome, labyrinth, intruder, vacation, yada, kmeans, ssca2) with selective macro-based instrumentation

* Concurrent data-structures (linked-list, skip-list, red-black tree, hash map) using the TM GCC atomic transaction construct

* STMBench7 (not yet available to "opt" hybrid backends, which require a dual instrumentation path) with selective macro-based instrumentation

* Memcached using the TM GCC atomic transaction construct

* TPC-C using the TM GCC atomic transaction construct


**Upon cloning the repository, make sure everything is bootstrapped:**

* compile the rapl-power library

* compile each STM under the "stms" folder

* each of the above requires only a "make clean; make" command within the folder


To run a given benchmark, use the build and run scripts that are present under each individual benchmark folder. 
These scripts expect several parameters:

1. the backend name, corresponding to the folder under "backends" (e.g., stm-swisstm)

2. the number of retries for HTM usage, which may be omitted (default value 5)

3. the retry policy to adopt in case of HTM capacity aborts, i.e., how the number of retries is updated upon a capacity
   abort, which may be omitted (possible values: 0, do not retry in htm and resort to the fallback path; 1, decrease by
   one; 2, divide by two; default value 1)
   
4. whether adaptation is enabled (0 is enabled)

5. which backend to use initially in the greentm case (0 = TinySTM, 1 = NOrec, 2 = TL2, 3 = SwissTM, 4 = RTM-SGL, 5 = HybridNOrec, 6 = HybridTL2)


Example commands:

+ Data-structures with NOrec: bash build-datastructures-gcc.sh 5 2 1 1
    * the last two parameteres are the relevant ones: 1 disables adaptivity and 1 chooses NOrec to start with (and stay there forever)
 
* STAMP with RTM-SGL, 10 retries, linear decay: bash build-datastructures-gcc.sh 10 1 1 4

+ STMBench7 with adaptivity enabled, starting with NOrec: bash build-stmbench7.sh 5 2 0 1
    * the second last number, 0, states that adaptivity is enabled
    * note: when enabling adaptivity, the maximum degree of parallelism is defined by the number of threads that the application is started with; as such, in such cases, it makes sense to start the application with as many threads as cores, and let the controller decide what's best to use.
    * note2: to run with adaptivity enabled, you should previously start the RecTM Recommender/Controller daemon; this entails simply executing the "daemon.sh" script within the "rectm" folder. Note that the training data used is for a Haswell machine with 4 cores (8 hyper-threads).
 
+ Memcached with adaptivity enabled, starting with HTM with 7 retries and give up on capacity abort: bash build-memcached-gcc.sh 7 0 0 4
    * note, the code generates only the binary for the server, which you should start with as many threads as cores/2 (assuming split usage of server and client threads in the same server) to take advantage of the adaptivity
    * for the client, use the memcslap utility that is available on linux repositories, and provide it with a concurrency degree that matches cores/2

Note that the above build commands compile the GCC library underlying PolyTM, which must be placed in the "LD_LIBRARY_PATH", so that it is found by the benchmark binaries. E.g.: "export LD_LIBRARY_PATH="<path-to-proteustm>/backends/greentm/gcc-abi/"

When running any benchmark, sudo privileges must be used, so that we may collect energy consumption metrics.
Alternatively, you may change the "RETURN_IF_NO_ENERGY" macro in the rapl-power/rapl.c file. 
Note that RAPL may complain about failing to initialize. In such cases, please execute the following command: sudo modprobe msr

