/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_LOG_CAPTURE_H
#define TESTS_LOG_CAPTURE_H

#include "include/binrec.h"

/*************************************************************************/
/*************************************************************************/

/**
 * log_capture:  Record log messages to a buffer.  Intended to be used as
 * the "log" function pointer in binrec_setup_t to record log messages
 * reported during tests.
 */
extern void log_capture(void *userdata, binrec_loglevel_t level,
                        const char *message);

/**
 * get_log_messages:  Return a string containing the accumulated log
 * messages, or NULL if no messages have been logged.  A newline is
 * appended to each message logged.
 */
extern const char *get_log_messages(void);

/**
 * clear_log_messages:  Clear all accumulated log messages.
 */
extern void clear_log_messages(void);

/*-----------------------------------------------------------------------*/

/**
 * EXPECT_ICE:  Check that the accumulated log message buffer contains an
 * internal compiler error with the given text, then clear the buffer.
 */
#define EXPECT_ICE(text)  do {                                          \
    const char * const _text = (text);                                  \
    ASSERT(_text);                                                      \
    const char * const _log = get_log_messages();                       \
    if (!_log) {                                                        \
        FAIL("Expected an error but none was detected");                \
    }                                                                   \
    const char *_ice = "[error] Internal compiler error:";              \
    if (strncmp(_log, _ice, strlen(_ice)) != 0) {                       \
        FAIL("Did not detect the expected error.  Log follows:\n%s", _log); \
    }                                                                   \
    const char *_colon = strstr(_log + strlen(_ice), ": ");             \
    ASSERT(_colon);                                                     \
    const char *_log_text = _colon + 2;                                 \
    const int _text_len = strlen(_text);                                \
    if (strncmp(_log_text, _text, _text_len) != 0                       \
     || _log_text[_text_len] != '\n'                                    \
     || _log_text[_text_len+1] != '\0') {                               \
        FAIL("Did not detect the expected error.  Log follows:\n%s", _log); \
    }                                                                   \
    clear_log_messages();                                               \
} while (0)

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_LOG_CAPTURE_H
