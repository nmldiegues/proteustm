/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/** \file
 * The main memcached header holding commonly used data
 * structures and function prototypes.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// [branch 012] Include support for registering onCommit handlers:
// #include "/home/spear/gcc/src/gcc_trunk/libitm/libitm.h"
#include "libitm.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <event.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include "protocol_binary.h"
#include "cache.h"

#include "sasl_defs.h"

// [branch 012] Include errno, so we can access it in our new perror code
#include <errno.h>

// [branch 008] Support for safe assertions.  Note that the evaluation of the
// expression occurs within the context of the transaction, but we don't
// commit before we call the safe_assert_internal code.
#if defined(NDEBUG)
#define tm_assert(e)   ((void)0)
#else
#include <stdlib.h>
#define tm_assert(e) ((e) ? (void)0 : tm_assert_internal(__FILE__, __LINE__, __func__, #e))
__attribute__((transaction_pure))
void tm_assert_internal(const char *filename, int linenum, const char *funcname, const char *sourceline);
#endif /* NDEBUG */

// [branch 008] This is our 'safe' printf-and-abort function
__attribute__((transaction_pure))
void tm_msg_and_die(const char* msg);

// [branch 009] Provide safe versions of memcmp, memcpy, strlen, strncmp,
//              strncpy, and realloc
__attribute__((transaction_safe))
int tm_memcmp(const void *s1, const void *s2, size_t n);
__attribute__((transaction_safe))
void *tm_memcpy(void *dst, const void *src, size_t len);
__attribute__((transaction_safe))
size_t tm_strlen(const char *s);
__attribute__((transaction_safe))
int tm_strncmp(const char *s1, const char *s2, size_t n);
__attribute__((transaction_safe))
char *tm_strncpy(char *dst, const char *src, size_t n);
__attribute__((transaction_safe))
void *tm_realloc(void *ptr, size_t size, size_t old_size);
__attribute__((transaction_safe))
void *tm_memset(void*, int, size_t);

// [branch 009b] a custom strncpy that reads via TM, and writes directly to a
//               location presumed to be thread-private (e.g., on the stack)
__attribute__((transaction_safe))
char *tm_strncpy_to_local(char *local_dst, const char *src, size_t n);

// [branch 009b] Provide safe versions of strtol, atoi, strtol, strchr, and
//               isspace
__attribute__((transaction_safe))
long int tm_strtol(const char *nptr, char **endptr, int base);
__attribute__((transaction_safe))
int tm_atoi(const char *nptr);
__attribute__((transaction_safe))
unsigned long long int tm_strtoull(const char *nptr, char **endptr, int base);
__attribute__((transaction_safe))
char *tm_strchr(const char *s, int c);
// [branch 009b] This can just be marked pure
__attribute__((transaction_pure))
int tm_isspace(int c);

// [branch 012] Provide support for oncommit handlers
__attribute__((transaction_pure))
void registerOnCommitHandler(void (*func)(void*), void *param);
void delayed_perror(int error_number, char *message);

/** Maximum length of a key. */
#define KEY_MAX_LENGTH 250

/** Size of an incr buf. */
#define INCR_MAX_STORAGE_LEN 24

#define DATA_BUFFER_SIZE 2048
#define UDP_READ_BUFFER_SIZE 65536
#define UDP_MAX_PAYLOAD_SIZE 1400
#define UDP_HEADER_SIZE 8
#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)
/* I'm told the max length of a 64-bit num converted to string is 20 bytes.
 * Plus a few for spaces, \r\n, \0 */
#define SUFFIX_SIZE 24

/** Initial size of list of items being returned by "get". */
#define ITEM_LIST_INITIAL 200

/** Initial size of list of CAS suffixes appended to "gets" lines. */
#define SUFFIX_LIST_INITIAL 20

/** Initial size of the sendmsg() scatter/gather array. */
#define IOV_LIST_INITIAL 400

/** Initial number of sendmsg() argument structures to allocate. */
#define MSG_LIST_INITIAL 10

/** High water marks for buffer shrinking */
#define READ_BUFFER_HIGHWAT 8192
#define ITEM_LIST_HIGHWAT 400
#define IOV_LIST_HIGHWAT 600
#define MSG_LIST_HIGHWAT 100

/* Binary protocol stuff */
#define MIN_BIN_PKT_LENGTH 16
#define BIN_PKT_HDR_WORDS (MIN_BIN_PKT_LENGTH/sizeof(uint32_t))

/* Initial power multiplier for the hash table */
#define HASHPOWER_DEFAULT 16

/* unistd.h is here */
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

/* Slab sizing definitions. */
#define POWER_SMALLEST 1
#define POWER_LARGEST  200
#define CHUNK_ALIGN_BYTES 8
#define MAX_NUMBER_OF_SLAB_CLASSES (POWER_LARGEST + 1)

/** How long an object can reasonably be assumed to be locked before
    harvesting it on a low memory condition. */
#define TAIL_REPAIR_TIME (3 * 3600)

/* warning: don't use these macros with a function, as it evals its arg twice */
#define ITEM_get_cas(i) (((i)->it_flags & ITEM_CAS) ? \
        (i)->data->cas : (uint64_t)0)

#define ITEM_set_cas(i,v) { \
    if ((i)->it_flags & ITEM_CAS) { \
        (i)->data->cas = v; \
    } \
}

#define ITEM_key(item) (((char*)&((item)->data)) \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_suffix(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_data(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (item)->nsuffix \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_ntotal(item) (sizeof(struct _stritem) + (item)->nkey + 1 \
         + (item)->nsuffix + (item)->nbytes \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define STAT_KEY_LEN 128
#define STAT_VAL_LEN 128

/** Append a simple stat with a stat name, value format and value */
#define APPEND_STAT(name, fmt, val) \
    append_stat(name, add_stats, c, fmt, val);

// [branch 011] Clone append_stat, use safe version
#define APPEND_STAT_LLU(name, fmt, val) \
    append_stat_llu(name, add_stats, c, fmt, val);
#define APPEND_STAT_U(name, fmt, val) \
    append_stat_u(name, add_stats, c, fmt, val);
#define APPEND_STAT_D(name, fmt, val) \
    append_stat_d(name, add_stats, c, fmt, val);
#define APPEND_STAT_LU(name, fmt, val) \
    append_stat_lu(name, add_stats, c, fmt, val);
#define APPEND_STAT_LD(name, fmt, val) \
    append_stat_ld(name, add_stats, c, fmt, val);
#define APPEND_STAT_S(name, fmt, val) \
    append_stat_s(name, add_stats, c, fmt, val);

/** Append an indexed stat with a stat name (with format), value format
    and value */
#define APPEND_NUM_FMT_STAT(name_fmt, num, name, fmt, val)          \
    klen = snprintf(key_str, STAT_KEY_LEN, name_fmt, num, name);    \
    vlen = snprintf(val_str, STAT_VAL_LEN, fmt, val);               \
    add_stats(key_str, klen, val_str, vlen, c);

// [branch 011] Clone APPEND_NUM_FMT_STAT to make it safe
#define APPEND_NUM_FMT_STAT_U(name_fmt, num, name, fmt, val)          \
    klen = tm_snprintf_ds(key_str, STAT_KEY_LEN, name_fmt, num, name);    \
    vlen = tm_snprintf_u(val_str, STAT_VAL_LEN, fmt, val);               \
    add_stats(key_str, klen, val_str, vlen, c);
#define APPEND_NUM_FMT_STAT_LLU(name_fmt, num, name, fmt, val)          \
    klen = tm_snprintf_ds(key_str, STAT_KEY_LEN, name_fmt, num, name);    \
    vlen = tm_snprintf_llu(val_str, STAT_VAL_LEN, fmt, val);               \
    add_stats(key_str, klen, val_str, vlen, c);

/** Common APPEND_NUM_FMT_STAT format. */
#define APPEND_NUM_STAT(num, name, fmt, val) \
    APPEND_NUM_FMT_STAT("%d:%s", num, name, fmt, val)
// [branch 011] Clone APPEND_NUM_STAT to make it safe
#define APPEND_NUM_STAT_U(num, name, fmt, val) \
    APPEND_NUM_FMT_STAT_U("%d:%s", num, name, fmt, val)
#define APPEND_NUM_STAT_LLU(num, name, fmt, val) \
    APPEND_NUM_FMT_STAT_LLU("%d:%s", num, name, fmt, val)

/**
 * Callback for any function producing stats.
 *
 * @param key the stat's key
 * @param klen length of the key
 * @param val the stat's value in an ascii form (e.g. text form of a number)
 * @param vlen length of the value
 * @parm cookie magic callback cookie
 */
// [branch 005] Force all instances of this function pointer to be safe to
//              call from atomic transactions
typedef void (*ADD_STAT)(const char *key, const uint16_t klen,
                         const char *val, const uint32_t vlen,
                         const void *cookie) __attribute__((transaction_safe));

/*
 * NOTE: If you modify this table you _MUST_ update the function state_text
 */
/**
 * Possible states of a connection.
 */
enum conn_states {
    conn_listening,  /**< the socket which listens for connections */
    conn_new_cmd,    /**< Prepare connection for next command */
    conn_waiting,    /**< waiting for a readable socket */
    conn_read,       /**< reading in a command line */
    conn_parse_cmd,  /**< try to parse a command from the input buffer */
    conn_write,      /**< writing out a simple response */
    conn_nread,      /**< reading in a fixed number of bytes */
    conn_swallow,    /**< swallowing unnecessary bytes w/o storing */
    conn_closing,    /**< closing this connection */
    conn_mwrite,     /**< writing out many items sequentially */
    conn_max_state   /**< Max state value (used for assertion) */
};

enum bin_substates {
    bin_no_state,
    bin_reading_set_header,
    bin_reading_cas_header,
    bin_read_set_value,
    bin_reading_get_key,
    bin_reading_stat,
    bin_reading_del_header,
    bin_reading_incr_header,
    bin_read_flush_exptime,
    bin_reading_sasl_auth,
    bin_reading_sasl_auth_data,
    bin_reading_touch_key,
};

enum protocol {
    ascii_prot = 3, /* arbitrary value. */
    binary_prot,
    negotiating_prot /* Discovering the protocol */
};

enum network_transport {
    local_transport, /* Unix sockets*/
    tcp_transport,
    udp_transport
};

enum item_lock_types {
    ITEM_LOCK_GRANULAR = 0,
    ITEM_LOCK_GLOBAL
};

#define IS_UDP(x) (x == udp_transport)

#define NREAD_ADD 1
#define NREAD_SET 2
#define NREAD_REPLACE 3
#define NREAD_APPEND 4
#define NREAD_PREPEND 5
#define NREAD_CAS 6

enum store_item_type {
    NOT_STORED=0, STORED, EXISTS, NOT_FOUND
};

enum delta_result_type {
    OK, NON_NUMERIC, EOM, DELTA_ITEM_NOT_FOUND, DELTA_ITEM_CAS_MISMATCH
};

/** Time relative to server start. Smaller than time_t on 64-bit systems. */
typedef unsigned int rel_time_t;

/** Stats stored per slab (and per thread). */
struct slab_stats {
    uint64_t  set_cmds;
    uint64_t  get_hits;
    uint64_t  touch_hits;
    uint64_t  delete_hits;
    uint64_t  cas_hits;
    uint64_t  cas_badval;
    uint64_t  incr_hits;
    uint64_t  decr_hits;
};

/**
 * Stats stored per-thread.
 */
struct thread_stats {
    // [branch 010] Removed per-thread stats mutexes; keeping object in
    //              struct for alignment
    pthread_mutex_t   defunct_mutex;
    uint64_t          get_cmds;
    uint64_t          get_misses;
    uint64_t          touch_cmds;
    uint64_t          touch_misses;
    uint64_t          delete_misses;
    uint64_t          incr_misses;
    uint64_t          decr_misses;
    uint64_t          cas_misses;
    uint64_t          bytes_read;
    uint64_t          bytes_written;
    uint64_t          flush_cmds;
    uint64_t          conn_yields; /* # of yields for connections (-R option)*/
    uint64_t          auth_cmds;
    uint64_t          auth_errors;
    struct slab_stats slab_stats[MAX_NUMBER_OF_SLAB_CLASSES];
};

/**
 * Global stats.
 */
struct stats {
    pthread_mutex_t mutex;
    unsigned int  curr_items;
    unsigned int  total_items;
    uint64_t      curr_bytes;
    unsigned int  curr_conns;
    unsigned int  total_conns;
    uint64_t      rejected_conns;
    unsigned int  reserved_fds;
    unsigned int  conn_structs;
    uint64_t      get_cmds;
    uint64_t      set_cmds;
    uint64_t      touch_cmds;
    uint64_t      get_hits;
    uint64_t      get_misses;
    uint64_t      touch_hits;
    uint64_t      touch_misses;
    uint64_t      evictions;
    uint64_t      reclaimed;
    time_t        started;          /* when the process was started */
    bool          accepting_conns;  /* whether we are currently accepting */
    uint64_t      listen_disabled_num;
    unsigned int  hash_power_level; /* Better hope it's not over 9000 */
    uint64_t      hash_bytes;       /* size used for hash tables */
    bool          hash_is_expanding; /* If the hash table is being expanded */
    uint64_t      expired_unfetched; /* items reclaimed but never touched */
    uint64_t      evicted_unfetched; /* items evicted but never touched */
    bool          slab_reassign_running; /* slab reassign in progress */
    uint64_t      slabs_moved;       /* times slabs were moved around */
};

#define MAX_VERBOSITY_LEVEL 2

/* When adding a setting, be sure to update process_stat_settings */
/**
 * Globally accessible settings as derived from the commandline.
 */
struct settings {
    size_t maxbytes;
    int maxconns;
    int port;
    int udpport;
    char *inter;
    int verbose;
    rel_time_t oldest_live; /* ignore existing items older than this */
    int evict_to_free;
    char *socketpath;   /* path to unix socket if using local socket */
    int access;  /* access mask (a la chmod) for unix domain socket */
    double factor;          /* chunk size growth factor */
    int chunk_size;
    int num_threads;        /* number of worker (without dispatcher) libevent threads to run */
    int num_threads_per_udp; /* number of worker threads serving each udp socket */
    char prefix_delimiter;  /* character that marks a key prefix (for stats) */
    int detail_enabled;     /* nonzero if we're collecting detailed stats */
    int reqs_per_event;     /* Maximum number of io to process on each
                               io-event. */
    bool use_cas;
    enum protocol binding_protocol;
    int backlog;
    int item_size_max;        /* Maximum item size, and upper end for slabs */
    bool sasl;              /* SASL on/off */
    bool maxconns_fast;     /* Whether or not to early close connections */
    bool slab_reassign;     /* Whether or not slab reassignment is allowed */
    int slab_automove;     /* Whether or not to automatically move slabs */
    int hashpower_init;     /* Starting hash power level */
};

extern struct stats stats;
extern time_t process_started;
extern struct settings settings;

#define ITEM_LINKED 1
#define ITEM_CAS 2

/* temp */
#define ITEM_SLABBED 4

#define ITEM_FETCHED 8

/**
 * Structure for storing items within memcached.
 */
typedef struct _stritem {
    struct _stritem *next;
    struct _stritem *prev;
    struct _stritem *h_next;    /* hash chain next */
    rel_time_t      time;       /* least recent access */
    rel_time_t      exptime;    /* expire time */
    int             nbytes;     /* size of data */
    // [branch 007] This field should not be accessed nontransactionally
    unsigned short  tm_refcount;
    uint8_t         nsuffix;    /* length of flags-and-length string */
    uint8_t         it_flags;   /* ITEM_* above */
    uint8_t         slabs_clsid;/* which slab class we're in */
    uint8_t         nkey;       /* key length, w/terminating null and padding */
    /* this odd type prevents type-punning issues when we do
     * the little shuffle to save space when not using CAS. */
    union {
        uint64_t cas;
        char end;
    } data[];
    /* if it_flags & ITEM_CAS we have 8 bytes CAS */
    /* then null-terminated key */
    /* then " flags length\r\n" (no terminating null) */
    /* then data with terminating \r\n (no terminating null; it's binary!) */
} item;

typedef struct {
    pthread_t thread_id;        /* unique ID of this thread */
    struct event_base *base;    /* libevent handle this thread uses */
    struct event notify_event;  /* listen event for notify pipe */
    int notify_receive_fd;      /* receiving end of notify pipe */
    int notify_send_fd;         /* sending end of notify pipe */
    struct thread_stats stats;  /* Stats generated by this thread */
    struct conn_queue *new_conn_queue; /* queue of new connections to handle */
    cache_t *suffix_cache;      /* suffix cache */
    uint8_t item_lock_type;     /* use fine-grained or global item lock */
} LIBEVENT_THREAD;

typedef struct {
    pthread_t thread_id;        /* unique ID of this thread */
    struct event_base *base;    /* libevent handle this thread uses */
} LIBEVENT_DISPATCHER_THREAD;

/**
 * The structure representing a connection into memcached.
 */
typedef struct conn conn;
struct conn {
    int    sfd;
    sasl_conn_t *sasl_conn;
    enum conn_states  state;
    enum bin_substates substate;
    struct event event;
    short  ev_flags;
    short  which;   /** which events were just triggered */

    char   *rbuf;   /** buffer to read commands into */
    char   *rcurr;  /** but if we parsed some already, this is where we stopped */
    int    rsize;   /** total allocated size of rbuf */
    int    rbytes;  /** how much data, starting from rcur, do we have unparsed */

    char   *wbuf;
    char   *wcurr;
    int    wsize;
    int    wbytes;
    /** which state to go into after finishing current write */
    enum conn_states  write_and_go;
    void   *write_and_free; /** free this memory after finishing writing */

    char   *ritem;  /** when we read in an item's value, it goes here */
    int    rlbytes;

    /* data for the nread state */

    /**
     * item is used to hold an item structure created after reading the command
     * line of set/add/replace commands, but before we finished reading the actual
     * data. The data is read into ITEM_data(item) to avoid extra copying.
     */

    void   *item;     /* for commands set/add/replace  */

    /* data for the swallow state */
    int    sbytes;    /* how many bytes to swallow */

    /* data for the mwrite state */
    struct iovec *iov;
    int    iovsize;   /* number of elements allocated in iov[] */
    int    iovused;   /* number of elements used in iov[] */

    struct msghdr *msglist;
    int    msgsize;   /* number of elements allocated in msglist[] */
    int    msgused;   /* number of elements used in msglist[] */
    int    msgcurr;   /* element in msglist[] being transmitted now */
    int    msgbytes;  /* number of bytes in current msg */

    item   **ilist;   /* list of items to write out */
    int    isize;
    item   **icurr;
    int    ileft;

    char   **suffixlist;
    int    suffixsize;
    char   **suffixcurr;
    int    suffixleft;

    enum protocol protocol;   /* which protocol this connection speaks */
    enum network_transport transport; /* what transport is used by this connection */

    /* data for UDP clients */
    int    request_id; /* Incoming UDP request ID, if this is a UDP "connection" */
    struct sockaddr request_addr; /* Who sent the most recent request */
    socklen_t request_addr_size;
    unsigned char *hdrbuf; /* udp packet headers */
    int    hdrsize;   /* number of headers' worth of space is allocated */

    bool   noreply;   /* True if the reply should not be sent. */
    /* current stats command */
    struct {
        char *buffer;
        size_t size;
        size_t offset;
    } stats;

    /* Binary protocol stuff */
    /* This is where the binary header goes */
    protocol_binary_request_header binary_header;
    uint64_t cas; /* the cas to return */
    short cmd; /* current command being processed */
    int opaque;
    int keylen;
    conn   *next;     /* Used for generating a list of conn structures */
    LIBEVENT_THREAD *thread; /* Pointer to the thread object serving this connection */
};


/* current time of day (updated periodically) */
// [branch 006] Replace volatile variable with "transactional" variable
extern rel_time_t tm_current_time;

/* TODO: Move to slabs.h? */
// [branch 006] Replace volatile variable with "transactional" variable
extern int tm_slab_rebalance_signal;

struct slab_rebalance {
    void *slab_start;
    void *slab_end;
    void *slab_pos;
    int s_clsid;
    int d_clsid;
    int busy_items;
    uint8_t done;
};

extern struct slab_rebalance slab_rebal;

/*
 * Functions
 */
void do_accept_new_conns(const bool do_accept);
// [branch 004b] This function is called from a relaxed transaction
// [branch 012b] With fprintf oncommit, this becomes safe
__attribute__((transaction_safe))
enum delta_result_type do_add_delta(conn *c, const char *key,
                                    const size_t nkey, const bool incr,
                                    const int64_t delta, char *buf,
                                    uint64_t *cas, const uint32_t hv);
// [branch 004b] This function is called from a relaxed transaction
// [branch 012b] With fprintf oncommit, this becomes safe
__attribute__((transaction_safe))
enum store_item_type do_store_item(item *item, int comm, conn* c, const uint32_t hv);
conn *conn_new(const int sfd, const enum conn_states init_state, const int event_flags, const int read_buffer_size, enum network_transport transport, struct event_base *base);
extern int daemonize(int nochdir, int noclose);

static inline int mutex_lock(pthread_mutex_t *mutex)
{
    while (pthread_mutex_trylock(mutex));
    return 0;
}

#define mutex_unlock(x) pthread_mutex_unlock(x)

#include "stats.h"
#include "slabs.h"
#include "assoc.h"
#include "items.h"
#include "trace.h"
#include "hash.h"
#include "util.h"

/*
 * Functions such as the libevent-related calls that need to do cross-thread
 * communication in multithreaded mode (rather than actually doing the work
 * in the current thread) are called via "dispatch_" frontends, which are
 * also #define-d to directly call the underlying code in singlethreaded mode.
 */

void thread_init(int nthreads, struct event_base *main_base);
int  dispatch_event_add(int thread, conn *c);
void dispatch_conn_new(int sfd, enum conn_states init_state, int event_flags, int read_buffer_size, enum network_transport transport);

/* Lock wrappers for cache functions that are called from main loop. */
enum delta_result_type add_delta(conn *c, const char *key,
                                 const size_t nkey, const int incr,
                                 const int64_t delta, char *buf,
                                 uint64_t *cas);
void accept_new_conns(const bool do_accept);
conn *conn_from_freelist(void);
bool  conn_add_to_freelist(conn *c);
int   is_listen_thread(void);
item *item_alloc(char *key, size_t nkey, int flags, rel_time_t exptime, int nbytes);
char *item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes);
void  item_flush_expired(void);
item *item_get(const char *key, const size_t nkey);
item *item_touch(const char *key, const size_t nkey, uint32_t exptime);
int   item_link(item *it);
void  item_remove(item *it);
// [branch 011b] This is called from a relaxed transaction
// [branch 012b] With fprintf oncommit, this becomes safe
__attribute__((transaction_safe))
int   item_replace(item *it, item *new_it, const uint32_t hv);
void  item_stats(ADD_STAT add_stats, void *c);
void  item_stats_totals(ADD_STAT add_stats, void *c);
void  item_stats_sizes(ADD_STAT add_stats, void *c);
void  item_unlink(item *it);
void  item_update(item *it);

// [branch 003b] Remove item_lock operations (keeping switch_item_lock_type for now)
void switch_item_lock_type(enum item_lock_types type);
// [branch 004] This function is called from a relaxed transaction
// [branch 007] Now this is safe, and uses a transaction internally
__attribute__((transaction_safe))
unsigned short tm_refcount_incr(unsigned short *refcount);
// [branch 004] This function is called from a relaxed transaction
// [branch 007] Now this is safe, and uses a transaction internally
__attribute__((transaction_safe))
unsigned short tm_refcount_decr(unsigned short *refcount);
// [branch 002] removed headers for STATS_LOCK and STATS_UNLOCK
void threadlocal_stats_reset(void);
// [branch 004] This function is called from a relaxed transaction
// [branch 010] With safe threadlocal stat locks, this becomes safe
__attribute__((transaction_safe))
void threadlocal_stats_aggregate(struct thread_stats *stats);
void slab_stats_aggregate(struct thread_stats *stats, struct slab_stats *out);

/* Stat processing functions */
// [branch 004] This function is called from a relaxed transaction
// [branch 011] No annotation any more
void append_stat(const char *name, ADD_STAT add_stats, conn *c,
                 const char *fmt, ...);

// [branch 011] safe clones of append_stat without varargs
__attribute__((transaction_safe))
void append_stat_llu(const char *name, ADD_STAT add_stats, conn *c,
                     const char *fmt, unsigned long long val);
__attribute__((transaction_safe))
void append_stat_u(const char *name, ADD_STAT add_stats, conn *c,
                   const char *fmt, unsigned int val);
__attribute__((transaction_safe))
void append_stat_d(const char *name, ADD_STAT add_stats, conn *c,
                   const char *fmt, int val);
__attribute__((transaction_safe))
void append_stat_lu(const char *name, ADD_STAT add_stats, conn *c,
                    const char *fmt, long val);
__attribute__((transaction_safe))
void append_stat_ld(const char *name, ADD_STAT add_stats, conn *c,
                    const char *fmt, long val);
__attribute__((transaction_safe))
void append_stat_s(const char *name, ADD_STAT add_stats, conn *c,
                   const char *fmt, const char *val);
__attribute__((transaction_safe))
void append_stat_ld_ld(const char *name, ADD_STAT add_stats, conn *c,
                       const char *fmt, long val1, long val2);
// [branch 011] These internal stat helper functions are useful in other
//              places, so they shouldn't be static
__attribute__((transaction_pure))
int tm_snprintf_d(char *str, size_t size, const char *format, int val);
__attribute__((transaction_pure))
int tm_snprintf_llu(char *str, size_t size, const char *format,
                    unsigned long long val);
__attribute__((transaction_pure))
int tm_snprintf_u(char *str, size_t size, const char *format,
                  unsigned int val);
__attribute__((transaction_pure))
int tm_snprintf_ds(char *str, size_t size, const char *format,
                   int val1, const char *val2);
// [branch 011] A snprintf variant for do_item_cachedump
__attribute__((transaction_pure))
int tm_snprintf_s_d_lu(char *str, size_t size, const char *format,
                       const char *val1, int val2, unsigned long val3);
__attribute__((transaction_pure))
int tm_snprintf_s_llu_llu_llu_llu(char *str, size_t size, const char *format,
                                 const char *val1, uint64_t val2, uint64_t val3,
                                 uint64_t val4, uint64_t val5);
// [branch 011b] This one is just for the b branch
__attribute__((transaction_pure))
int tm_snprintf_d_d(char *str, size_t size, const char *format, int val1, int val2);

enum store_item_type store_item(item *item, int comm, conn *c);

#if HAVE_DROP_PRIVILEGES
extern void drop_privileges(void);
#else
#define drop_privileges()
#endif

/* If supported, give compiler hints for branch prediction. */
#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
