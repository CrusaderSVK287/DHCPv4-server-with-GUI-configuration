#ifndef __LOGGER__H_
#define __LOGGER__H_

#include <cclog.h>
#include <cclog_macros.h>

/* default verbosity is 10*/
enum LOG_LEVELS {
    LOG_CRITICAL,  /* verbosity 1, critical process ending errors */
    LOG_ERROR,     /* verbosity 2, errors that can be recovered from */
    LOG_WARN,      /* verbosity 3, warnings */
    LOG_MSG,       /* verbosity 4, generic log message */
    LOG_INFO,      /* verbosity 5, information that may be needed for debugging */
    LOG_UNIX       /* verbosity 5, For unix server */
#ifdef DEBUG
    /* These logs will only be created when DEBUG is defined. 
     * Dont use directly with cclog macro, use macros defined below
     */
    LOG_TRACE,     /* verbosity 1, used to trace functions */ 
    LOG_DEBUG,     /* verbosity 1, used to write debug messages */
#endif
};

#ifdef DEBUG
#define cclog_TRACE(MSG, ...) cclog(LOG_TRACE, NULL, MSG, ## __VA_ARGS__)
#define cclog_DEBUG(MSG, ...) cclog(LOG_DEBUG, NULL, MSG, ## __VA_ARGS__)
#else 
#define cclog_TRACE(...)
#define cclog_DEBUG(...)
#endif

// Bugfix of the library macros

#ifdef if_true_log 
#undef if_true_log
#define if_true_log(val, label, log_level, priv, msg, ...) {\
    if (val == true) {\
        cclog(log_level, priv, msg, ## __VA_ARGS__);\
        goto label;\
    }\
}
#endif

#ifdef if_false_log 
#undef if_false_log
#define if_false_log(val, label, log_level, priv, msg, ...) {\
    if (val == false) {\
        cclog(log_level, priv, msg, ## __VA_ARGS__);\
        goto label;\
    }\
}
#endif

#ifdef if_not_null_log 
#undef if_not_null_log
#define if_not_null_log(val, label, log_level, priv, msg, ...) {\
    if (val != NULL) {\
        cclog(log_level, priv, msg, ## __VA_ARGS__);\
        goto label;\
    }\
}
#endif

/* Initialises logging for dhcp server */
int init_logging();

/* Deinitialises logging for dhcp server */
void uninit_logging();

#endif /* __LOGGER__H_ */
