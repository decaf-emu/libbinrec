/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "tests/log-capture.h"
#include <stdio.h>


static char *log_messages = NULL;

void log_capture(UNUSED void *userdata, binrec_loglevel_t level,
                 const char *message)
{
    static const char * const level_prefix[] = {
        [BINREC_LOGLEVEL_INFO   ] = "info",
        [BINREC_LOGLEVEL_WARNING] = "warning",
        [BINREC_LOGLEVEL_ERROR  ] = "error",
    };
    const int current_len = log_messages ? strlen(log_messages) : 0;
    const int message_len =
        snprintf(NULL, 0, "[%s] %s\n", level_prefix[level], message);
    const int new_size = current_len + message_len + 1;
    ASSERT(log_messages = realloc(log_messages, new_size));
    ASSERT(snprintf(log_messages + current_len, message_len + 1, "[%s] %s\n",
                    level_prefix[level], message) == message_len);
}

const char *get_log_messages(void)
{
    return log_messages;
}

void clear_log_messages(void)
{
    free(log_messages);
    log_messages = NULL;
}

bool find_ice(const char *log, const char *text)
{
    ASSERT(log);
    ASSERT(text);

    while (*log) {
        const char *ice = "[error] Internal compiler error: ";
        if (strncmp(log, ice, strlen(ice)) == 0) {
            const char *colon = strstr(log + strlen(ice), ": ");
            ASSERT(colon);
            const char *log_text = colon + 2;
            const int text_len = strlen(text);
            if (strncmp(log_text, text, text_len) == 0
             && log_text[text_len] == '\n') {
                return true;
            }
        }

        log += strcspn(log, "\n");
        if (*log) {
            log++;
        }
    }

    return false;
}
