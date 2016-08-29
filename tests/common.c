/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "tests/common.h"


/**
 * _diff_strings:  Search for differences between two strings and print
 * them to the given file.  Helper for the EXPECT_STREQ macro in common.h.
 *
 * This function implements an extremely simplistic "diff" algorithm to
 * show differences between two multi-line strings, such as when checking
 * RTL disassembly in a test.  The algorithm assumes the two strings will
 * be roughly similar, and it does not try very hard to minimize the set of
 * differences reported.
 *
 * [Parameters]
 *     f: File to which to print differences (typically stdout or stderr).
 *     from, to: Strings to compare.
 */
void _diff_strings(FILE *f, const char *from, const char *to)
{
    /* Beginning-of-line pointers for each of the last 4 lines in each
     * string (for context printing).  The most recently read line is in
     * {from_to}_bol[0], and successive array elements point to preceding
     * lines. */
    const char *from_bol[4], *to_bol[4];
    for (int i = 0; i < lenof(from_bol); i++) {
        from_bol[i] = NULL;
        to_bol[i] = NULL;
    }
    int from_line = 0, to_line = 0;  // Line number of {from,to}_bol[0]
    bool in_diff = false;  // Are we in the middle of printing a diff hunk?
    int match_count = 0;  // Number of consecutive matching lines (if in_diff)

    while (*from && *to) {
        memmove(&from_bol[1], &from_bol[0],
                sizeof(*from_bol) * (lenof(from_bol) - 1));
        from_bol[0] = from;
        from_line++;
        const char *from_eol = from + strcspn(from, "\n");
        const int from_len = from_eol - from;
        const bool from_has_lf = (*from_eol != 0);
        if (from_has_lf) {
            from_eol++;
        }
        ASSERT(from_eol > from);
        from = from_eol;

        memmove(&to_bol[1], &to_bol[0],
                sizeof(*to_bol) * (lenof(to_bol) - 1));
        to_bol[0] = to;
        to_line++;
        const char *to_eol = to + strcspn(to, "\n");
        const int to_len = to_eol - to;
        const bool to_has_lf = (*to_eol != 0);
        if (to_has_lf) {
            to_eol++;
        }
        ASSERT(to_eol > to);
        to = to_eol;

        const bool lines_match =
            (from_eol - from_bol[0] == to_eol - to_bol[0]
             && strncmp(from_bol[0], to_bol[0], from_eol - from_bol[0]) == 0);

        if (in_diff) {
            if (lines_match) {
                match_count++;
                if (match_count <= 3) {
                    fprintf(f, " %.*s\n", from_len, from_bol[0]);
                    if (!from_has_lf) {
                        ASSERT(!to_has_lf);
                        fprintf(f, "\\ No newline at end of file\n");
                    }
                }
                if (match_count > 6) {
                    /* We're now at least 4 lines away from the last
                     * context line, so exit this hunk. */
                    in_diff = false;
                }
            } else {
                if (match_count > 3) {
                    /* Print the matching lines we skipped over at the
                     * previous tentative end of the hunk. */
                    for (int i = match_count - 1; i >= 1; i--) {
                        ASSERT(from_bol[i-1][-1] == '\n');
                        const int line_len = from_bol[i-1] - from_bol[i] - 1;
                        fprintf(f, " %.*s\n", line_len, from_bol[i]);
                    }
                }
                match_count = 0;
                fprintf(f, "-%.*s\n", from_len, from_bol[0]);
                if (!from_has_lf) {
                    fprintf(f, "\\ No newline at end of file\n");
                }
                fprintf(f, "+%.*s\n", to_len, to_bol[0]);
                if (!to_has_lf) {
                    fprintf(f, "\\ No newline at end of file\n");
                }
            }
        } else {
            if (!lines_match) {
                in_diff = true;
                match_count = 0;
                if (from_line >= 5) {
                    fprintf(f, "...\n");
                }
                for (int i = min(from_line - 1, 3); i >= 1; i--) {
                    ASSERT(from_bol[i-1][-1] == '\n');
                    const int line_len = from_bol[i-1] - from_bol[i] - 1;
                    fprintf(f, " %.*s\n", line_len, from_bol[i]);
                }
                fprintf(f, "-%.*s\n", from_len, from_bol[0]);
                if (!from_has_lf) {
                    fprintf(f, "\\ No newline at end of file\n");
                }
                fprintf(f, "+%.*s\n", to_len, to_bol[0]);
                if (!to_has_lf) {
                    fprintf(f, "\\ No newline at end of file\n");
                }
            }
        }
    }  // while (*from && *to)

    if (*from || *to) {
        if (!in_diff || match_count > 3) {
            int print_count;
            if (!in_diff) {
                fprintf(f, "...\n");
                print_count = min(from_line - 1, 3);
            } else {
                print_count = match_count - 3;
            }
            for (int i = print_count; i >= 1; i--) {
                ASSERT(from_bol[i-1][-1] == '\n');
                const int line_len = from_bol[i-1] - from_bol[i] - 1;
                fprintf(f, " %.*s\n", line_len, from_bol[i]);
            }
        }

        const char *s = *from ? from : to;
        const char prefix = *from ? '-' : '+';
        while (*s) {
            const char *eol = s + strcspn(s, "\n");
            const int line_len = eol - s;
            fprintf(f, "%c%.*s\n", prefix, line_len, s);
            if (*eol) {
                eol++;
            } else {
                fprintf(f, "\\ No newline at end of file\n");
            }
            s = eol;
        }
    } else {  // !*from && !*to
        if (!in_diff || match_count > 3) {
            fprintf(f, "...\n");
        }
    }
}
