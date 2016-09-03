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

#ifndef BINREC_H
    #include "include/binrec.h"
#endif
#include <stdbool.h>

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

/**
 * find_ice:  Look for the given text as an internal compiler error (ICE)
 * in the given log message buffer.  Helper for the EXPECT_ICE macro.
 *
 * [Parameters]
 *     log: Log message buffer returned from get_log_messages().
 *     text: Text of the expected ICE, excluding the "Internal compiler
 *         error:" and filename / line number header.
 * [Return value]
 *     True if the error was found, false if not.
 */
extern bool find_ice(const char *log, const char *text);

/*-----------------------------------------------------------------------*/

/**
 * EXPECT_ICE:  Check that the accumulated log message buffer contains an
 * internal compiler error with the given text, then clear the buffer.
 * The passed-in string should not contain the "Internal compiler error:"
 * header or a trailing newline.
 */
#define EXPECT_ICE(text)  do {                          \
    const char * const _text = (text);                  \
    ASSERT(_text);                                      \
    const char * const _log = get_log_messages();       \
    if (!_log) {                                        \
        FAIL("Expected an error but none was detected"); \
    } else if (!find_ice(_log, _text)) {                \
        FAIL("Did not detect the expected error.  Log follows:\n%s", _log); \
    }                                                   \
    clear_log_messages();                               \
} while (0)

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_LOG_CAPTURE_H
