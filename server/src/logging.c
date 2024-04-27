
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define FORMAT_FATAL "[${DATE} ${TIME}] ${FILE}:${LINE}: *** FATAL ERROR *** ${MSG} (${ERMSG})"
#define FORMAT_ERROR "[${DATE} ${TIME}] ${FILE}:${LINE}: ERROR !!! ${MSG}"
#define FORMAT_WARNING "[${DATE} ${TIME}] ${FILE}: WARNING ! ${MSG}"
#define FORMAT_UNIX "[${DATE} ${TIME}] UNIX: ${MSG} - ${ERMSG}"

#ifdef DEBUG
#define FORMAT_TRACE "===TRACE=== [${UPTIME}] ${FILE} ${FUNCTION} ${LINE}: ${MSG}"
#define FORMAT_DEBUG "===DEBUG=== [${UPTIME}] ${FILE}${LINE}: errno (${ERRNO}: ${ERMSG}) ${MSG}"
#endif

int init_logging()
{
        int rv = 1;

        mkdir("/var/log", 0777);
        mkdir("/var/log/dhcps", 0777);
        if_failed(cclogger_init(LOGGING_MULTIPLE_FILES, "/var/log/dhcps/dhcp-log", "dhcps"), exit);
        if_failed(cclogger_set_default_message_format("[${DATE} ${TIME}] ${FILE}: ${MSG}"), exit);
        
        cclogger_set_verbosity_level(4);
        cclogger_reset_log_levels();
        
        /* Critical error */
        if_failed(cclogger_add_log_level(true, true, CCLOG_TTY_CLR_MAG, NULL, FORMAT_FATAL, 1), exit);
        /* Error */
        if_failed(cclogger_add_log_level(true, true, CCLOG_TTY_CLR_RED, NULL, FORMAT_ERROR, 2), exit);
        /* Warning */
        if_failed(cclogger_add_log_level(true, true, CCLOG_TTY_CLR_YEL, NULL, FORMAT_WARNING, 3), exit);
        /* Log */
        if_failed(cclogger_add_log_level(true, false, CCLOG_TTY_CLR_DEF, NULL, NULL, 4), exit);
        /* Info - generaly not needed, hence high verbosity level required */
        if_failed(cclogger_add_log_level(true, false, CCLOG_TTY_CLR_DEF, NULL, NULL, 5), exit);
        /* UNIX - only used for information regarding UNIX server */
        if_failed(cclogger_add_log_level(true, true, CCLOG_TTY_CLR_DEF, NULL, FORMAT_UNIX, 5), exit);
        
#ifdef DEBUG
        /* Trace */
        if_failed(cclogger_add_log_level(true, true, CCLOG_TTY_CLR_WHT, NULL, FORMAT_TRACE, 1), exit);
        /* Debug */
        if_failed(cclogger_add_log_level(true, true, CCLOG_TTY_CLR_WHT, NULL, FORMAT_DEBUG, 1), exit);
#endif

        cclog(LOG_MSG, NULL, "Logger initialised");

        rv = 0;
exit: 
        return rv;
}

void uninit_logging()
{
        cclog(LOG_MSG, NULL, "Uninitialising logger");
        cclogger_uninit();
}
