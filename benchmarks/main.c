/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "benchmarks/blobs.h"
#include "benchmarks/common.h"
#include "tests/execute.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>

/*************************************************************************/
/************ Lists of available architectures and benchmarks ************/
/*************************************************************************/

/* Enumeration of available guest architectures. */
typedef enum GuestArch {
    GUEST_ARCH_INVALID = -1,
    GUEST_ARCH_PPC_7XX,
    GUEST_ARCH__NUM,  // Number of guest architectures.
    GUEST_ARCH_NATIVE = GUEST_ARCH__NUM  // Dummy value for command line arg.
} GuestArch;

/* Names of guest architectures, for command line parsing. */
static const char * const arch_names[GUEST_ARCH__NUM] = {
    [GUEST_ARCH_PPC_7XX] = "ppc32",
};

/*-----------------------------------------------------------------------*/

/* Declarations of native benchmark entry points. */

extern void dhry_noopt_Init(void);
extern int dhry_noopt_Main(int count);

extern void dhry_opt_Init(void);
extern int dhry_opt_Main(int count);

/*-----------------------------------------------------------------------*/

/* List of and data for available benchmarks. */

typedef struct Benchmark {
    const char *id;     // Identifier for use in the command line.
    const char *name;   // Human-readable name.

    /* Benchmark initialization routine for native benchmarking, or NULL
     * if no separate initialization is required. */
    void (*native_init)(void);
    /* Benchmark entry point for native benchmarking.  Takes an iteration
     * count and returns success (nonzero) or failure (zero). */
    int (*native_main)(int count);
    /* Benchmark finalization routine for native benchmarking, or NULL
     * if no separate finalization is required. */
    void (*native_fini)(void);

    /* Guest code blobs for each guest architecture, using the same
     * function signature for the entry point. */
    const Blob *guest_code[GUEST_ARCH__NUM];
} Benchmark;

static const Benchmark benchmarks[] = {
    {.id = "dhry_noopt", .name = "Dhrystone (unoptimized)",
     .native_init = dhry_noopt_Init,
     .native_main = dhry_noopt_Main,
     .guest_code = {
         [GUEST_ARCH_PPC_7XX] = &ppc32_dhry_noopt,
     },
    },
    {.id = "dhry_opt", .name = "Dhrystone (optimized)",
     .native_init = dhry_opt_Init,
     .native_main = dhry_opt_Main,
     .guest_code = {
         [GUEST_ARCH_PPC_7XX] = &ppc32_dhry_opt,
     },
    },
};

/*-----------------------------------------------------------------------*/

/* Parameters from the command line. */

static GuestArch arch;
static const Benchmark *benchmark;
static int count;
static unsigned int opt_common;
static unsigned int opt_guest;
static unsigned int opt_host;
static bool dump;
static bool quiet;
static bool verbose;

/*************************************************************************/
/*************************** Utility routines ****************************/
/*************************************************************************/

/**
 * find_guest_arch:  Return the enumerator (nonnegative) for the given
 * guest architecture, or GUEST_ARCH_INVALID (-1) if the architecture name
 * is invalid.
 */
static int find_guest_arch(const char *name)
{
    for (int i = 0; i < GUEST_ARCH__NUM; i++) {
        if (strcmp(name, arch_names[i]) == 0) {
            return arch;
        }
    }
    return GUEST_ARCH_INVALID;
}

/*-----------------------------------------------------------------------*/

/**
 * find_benchmark:  Return the benchmark data structure for the given
 * benchmark identifier, or NULL if the identifier is invalid.
 */
static const Benchmark *find_benchmark(const char *id)
{
    for (int i = 0; i < lenof(benchmarks); i++) {
        if (strcmp(id, benchmarks[i].id) == 0) {
            return &benchmarks[i];
        }
    }
    return NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * process_command_line:  Process command line arguments.
 *
 * [Parameters]
 *     argc: Command line argument count.
 *     argv: Command line argument vector.
 * [Return value]
 *     True on success, false on error.
 */
static bool process_command_line(int argc, char **argv)
{
    const char *arg_arch = NULL;
    const char *arg_benchmark = NULL;
    const char *arg_count = NULL;
    int opt_level = 0;

    for (int argi = 1; argi < argc; argi++) {

        if (strcmp(argv[argi], "help") == 0
         || strcmp(argv[argi], "-h") == 0
         || strncmp(argv[argi], "--help", strlen(argv[argi])) == 0) {
            fprintf(stderr, "Usage: %s [OPTIONS] ARCH-NAME BENCHMARK"
                    " ITERATION-COUNT\n", argv[0]);
            fprintf(stderr, "\nOptions:\n"
                    "    -d          Dump the translated code to disk.\n"
                    "    -G<NAME>    Enable specific guest optimizations.\n"
                    "        -Gppc-cr-stores      Eliminate dead stores to CR bits\n"
                    "    -H<NAME>    Enable specific host optimizations.\n"
                    "        -Hx86-address-op     Address operand optimization\n"
                    "        -Hx86-branch-align   Branch target alignment\n"
                    "        -Hx86-cond-codes     Condition code reuse\n"
                    "        -Hx86-fixed-regs     Smarter register allocation\n"
                    "        -Hx86-store-imm      Use mem-imm form for constant stores\n"
                    "    -O[LEVEL]   Select optimization level.\n"
                    "        -O0         Disable all optimizations (default).\n"
                    "        -O, -O1     Enable lightweight optimizations.\n"
                    "        -O2         Enable stronger but more expensive optimizations.\n"
                    "    -O<NAME>    Enable specific global optimizations.\n"
                    "        -Obasic              Basic optimizations\n"
                    "        -Odecondition        Branch deconditioning\n"
                    "        -Odeep-data-flow     Deep data flow analysis (expensive)\n"
                    "        -Odse                Dead store elimination\n"
                    "        -Ofold-constants     Constant folding\n"
                    "    -q          Suppress all log messages from translation.\n"
                    "    -v          Output verbose log messages from translation.\n"
            );
            fprintf(stderr, "\nValid architectures:\n    native\n");
            for (int i = 0; i < GUEST_ARCH__NUM; i++) {
                fprintf(stderr, "    %s\n", arch_names[i]);
            }
            fprintf(stderr, "\nAvailable benchmarks:\n");
            int max_len = 0;
            for (int i = 0; i < lenof(benchmarks); i++) {
                max_len = max(max_len, (int)strlen(benchmarks[i].id));
            }
            for (int i = 0; i < lenof(benchmarks); i++) {
                fprintf(stderr, "    %-*s  %s\n",
                        max_len, benchmarks[i].id, benchmarks[i].name);
            }
            fprintf(stderr,
                    "\n"
                    "Prints the time consumed by the benchmark to stdout in the format:\n"
                    "    TOTAL-TIME = USER-TIME + SYSTEM-TIME\n"
                    "where TOTAL-TIME, USER-TIME, and SYSTEM-TIME are in seconds.\n"
                    "If the benchmark does not complete successfully, nothing is printed\n"
                    "to stdout, and an appropriate error message is written to stderr.\n");
            return false;

        } else if (*argv[argi] == '-') {

            if (strcmp(argv[argi], "-d") == 0) {
                dump = true;

            } else if (argv[argi][1] == 'G') {
                const char *name = &argv[argi][2];
                if (!*name) {
                    fprintf(stderr, "Missing guest optimization flag\n");
                    goto usage;
                } else if (strcmp(name, "ppc-cr-stores") == 0) {
                    opt_guest |= BINREC_OPT_G_PPC_TRIM_CR_STORES;
                } else {
                    fprintf(stderr, "Unknown guest optimization flag %s\n",
                            name);
                    goto usage;
                }

            } else if (argv[argi][1] == 'H') {
                const char *name = &argv[argi][2];
                if (!*name) {
                    fprintf(stderr, "Missing host optimization flag\n");
                    goto usage;
                } else if (strcmp(name, "x86-address-op") == 0) {
                    opt_common |= BINREC_OPT_H_X86_ADDRESS_OPERANDS;
                } else if (strcmp(name, "x86-branch-align") == 0) {
                    opt_common |= BINREC_OPT_H_X86_BRANCH_ALIGNMENT;
                } else if (strcmp(name, "x86-cond-codes") == 0) {
                    opt_common |= BINREC_OPT_H_X86_CONDITION_CODES;
                } else if (strcmp(name, "x86-fixed-regs") == 0) {
                    opt_common |= BINREC_OPT_H_X86_FIXED_REGS;
                } else if (strcmp(name, "x86-merge-regs") == 0) {
                    opt_common |= BINREC_OPT_H_X86_MERGE_REGS;
                } else if (strcmp(name, "x86-store-imm") == 0) {
                    opt_common |= BINREC_OPT_H_X86_STORE_IMMEDIATE;
                } else {
                    fprintf(stderr, "Unknown host optimization flag %s\n",
                            name);
                    goto usage;
                }

            } else if (argv[argi][1] == 'O') {
                const char *name = &argv[argi][2];
                if (!*name || (*name >= '0' && *name <= '9' && !name[1])) {
                    opt_level = *name ? *name - '0' : 1;
                } else if (strcmp(name, "basic") == 0) {
                    opt_common |= BINREC_OPT_BASIC;
                } else if (strcmp(name, "decondition") == 0) {
                    opt_common |= BINREC_OPT_DECONDITION;
                } else if (strcmp(name, "deep-data-flow") == 0) {
                    opt_common |= BINREC_OPT_DEEP_DATA_FLOW;
                } else if (strcmp(name, "dse") == 0) {
                    opt_common |= BINREC_OPT_DSE;
                } else if (strcmp(name, "fold-constants") == 0) {
                    opt_common |= BINREC_OPT_FOLD_CONSTANTS;
                } else {
                    fprintf(stderr, "Unknown global optimization flag %s\n",
                            name);
                    goto usage;
                }

            } else if (strcmp(argv[argi], "-q") == 0) {
                quiet = true;

            } else if (strcmp(argv[argi], "-v") == 0) {
                verbose = true;

            } else {
                fprintf(stderr, "Unknown option %s\n", argv[argi]);
                goto usage;
            }

        } else if (!arg_arch) {
            arg_arch = argv[argi];
        } else if (!arg_benchmark) {
            arg_benchmark = argv[argi];
        } else if (!arg_count) {
            arg_count = argv[argi];
        } else {
            fprintf(stderr, "Wrong number of arguments\n");
            goto usage;
        }
    }

    if (!arg_count) {
        fprintf(stderr, "Wrong number of arguments\n");
      usage:
        fprintf(stderr, "Usage: %s ARCH-NAME BENCHMARK ITERATION-COUNT\n",
                argv[0]);
        fprintf(stderr, "Use %s -h for help.\n", argv[0]);
        return false;
    }

    if (strcmp(arg_arch, "native") == 0) {
        arch = GUEST_ARCH_NATIVE;
    } else {
        arch = find_guest_arch(arg_arch);
        if (arch == GUEST_ARCH_INVALID) {
            fprintf(stderr, "Unknown architecture name: %s\n", arg_arch);
            goto usage;
        }
    }

    benchmark = find_benchmark(arg_benchmark);
    if (!benchmark) {
        fprintf(stderr, "Unknown benchmark: %s\n", arg_benchmark);
        goto usage;
    }

    char *eos;
    count = (int)strtol(arg_count, &eos, 0);
    if (*eos || count <= 0) {
        fprintf(stderr, "Invalid iteration count: %s\n", arg_count);
        goto usage;
    }

    if (opt_level > 0) {
        const binrec_arch_t native_arch = binrec_native_arch();
        if (opt_level >= 1) {
            opt_common |= BINREC_OPT_BASIC
                        | BINREC_OPT_DECONDITION
                        | BINREC_OPT_DSE
                        | BINREC_OPT_FOLD_CONSTANTS;
            if (arch == GUEST_ARCH_PPC_7XX) {
                opt_guest |= BINREC_OPT_G_PPC_TRIM_CR_STORES;
            }
            if (native_arch == BINREC_ARCH_X86_64_SYSV
             || native_arch == BINREC_ARCH_X86_64_WINDOWS) {
                opt_host |= BINREC_OPT_H_X86_BRANCH_ALIGNMENT
                          | BINREC_OPT_H_X86_CONDITION_CODES
                          | BINREC_OPT_H_X86_FIXED_REGS
                          | BINREC_OPT_H_X86_STORE_IMMEDIATE;
            }
        }
        if (opt_level >= 2) {
            opt_common |= BINREC_OPT_DEEP_DATA_FLOW;
            if (native_arch == BINREC_ARCH_X86_64_SYSV
             || native_arch == BINREC_ARCH_X86_64_WINDOWS) {
                opt_host |= BINREC_OPT_H_X86_ADDRESS_OPERANDS
                          | BINREC_OPT_H_X86_MERGE_REGS;
            }
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * call_native:  Call the native entry point for the selected benchmark,
 * and return its result.
 *
 * [Return value]
 *     True if the benchmark completed successfully, false on error.
 */
static bool call_native(void)
{
    if (benchmark->native_init) {
        (*benchmark->native_init)();
    }
    const bool result = (*benchmark->native_main)(count);
    if (!result) {
        fprintf(stderr, "Benchmark reported failure\n");
    }
    if (benchmark->native_fini) {
        (*benchmark->native_fini)();
    }
    return result;
}

/*-----------------------------------------------------------------------*/

/**
 * log_callback:  Log callback for translation.
 */
static void log_callback(UNUSED void *userdata, binrec_loglevel_t level,
                         const char *message)
{
    static const char * const level_prefix[] = {
        [BINREC_LOGLEVEL_INFO   ] = "info",
        [BINREC_LOGLEVEL_WARNING] = "warning",
        [BINREC_LOGLEVEL_ERROR  ] = "error",
    };
    if (verbose || level >= BINREC_LOGLEVEL_WARNING) {
        fprintf(stderr, "[%s] %s\n", level_prefix[level], message);
    }
}


/*-----------------------------------------------------------------------*/

/**
 * configure_binrec:  Configure a libbinrec handle for translation.
 * Callback passed to call_guest_code().
 */
static void configure_binrec(binrec_t *handle)
{
    binrec_set_optimization_flags(handle, opt_common, opt_guest, opt_host);
}

/*-----------------------------------------------------------------------*/

/**
 * dump_translated_code:  Callback passed to call_guest_code() to dump
 * each unit of translated code to a file.
 */
static void dump_translated_code(uint32_t address, void *code, long code_size)
{
    char filename[16];
    ASSERT(snprintf(filename, sizeof(filename), "%08X.bin", address)
           < (int)sizeof(filename));
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "open(%s): %s\n", filename, strerror(errno));
    } else {
        if (fwrite(code, code_size, 1, f) != 1) {
            fprintf(stderr, "write(%s): %s\n", filename, strerror(errno));
        }
        fclose(f);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * call_guest:  Call the entry point for the selected guest architecture
 * and benchmark, and return its result.
 *
 * [Return value]
 *     True if the benchmark completed successfully, false on error.
 */
static bool call_guest(void)
{
    binrec_arch_t binrec_arch;
    void *state;
    uint32_t *arg_ptr;
    uint32_t *retval_ptr;

    const Blob *blob = benchmark->guest_code[arch];
    if (!blob) {
        fprintf(stderr, "Blob missing for benchmark %s, architecture %s\n",
                benchmark->id, arch_names[arch]);
        return false;
    }
    void *memory = alloc_guest_memory(blob->reserve);
    if (!memory) {
        fprintf(stderr, "Failed to reserve guest memory (0x%"PRIX64" bytes)\n",
                blob->reserve);
    }
    memcpy((char *)memory + blob->base, blob->data, blob->size);

    switch (arch) {
      case GUEST_ARCH_PPC_7XX: {
        static PPCState ppc_state;
        memset(&ppc_state, 0, sizeof(ppc_state));
        ppc_state.gpr[1] = blob->base - 8;  // Leave space for first LR store!
        arg_ptr = &ppc_state.gpr[3];
        retval_ptr = &ppc_state.gpr[3];
        binrec_arch = BINREC_ARCH_PPC_7XX;
        state = &ppc_state;
        break;
      }
      default:
        fprintf(stderr, "Invalid guest architecture %d\n", arch);
        return false;
    }

    bool success = false;

    if (blob->init && !call_guest_code(binrec_arch, state, memory, blob->init,
                                       quiet ? NULL : log_callback,
                                       configure_binrec,
                                       dump ? dump_translated_code : NULL)) {
        fprintf(stderr, "Guest code init() execution failed\n");
        goto done;
    }

    *arg_ptr = count;
    if (!call_guest_code(binrec_arch, state, memory, blob->main,
                         quiet ? NULL : log_callback, configure_binrec,
                         dump ? dump_translated_code : NULL)) {
        fprintf(stderr, "Guest code main() execution failed\n");
        goto done;
    }

    success = (*retval_ptr != 0);
    if (!success) {
        fprintf(stderr, "Benchmark reported failure\n");
    }

    if (blob->fini && !call_guest_code(binrec_arch, state, memory, blob->fini,
                                       quiet ? NULL : log_callback,
                                       configure_binrec,
                                       dump ? dump_translated_code : NULL)) {
        fprintf(stderr, "Guest code fini() execution failed\n");
        success = false;
        goto done;
    }

  done:
    free_guest_memory(memory, blob->reserve);
    return success;
}

/*************************************************************************/
/************************** Program entry point **************************/
/*************************************************************************/

int main(int argc, char **argv)
{
    if (!process_command_line(argc, argv)) {
        return EXIT_FAILURE;
    }

    double start_user, start_sys;
    get_cpu_time(&start_user, &start_sys);

    int result;
    if (arch == GUEST_ARCH_NATIVE) {
        result = call_native();
    } else {
        result = call_guest();
    }
    if (!result) {
        return EXIT_FAILURE;
    }

    double end_user, end_sys;
    get_cpu_time(&end_user, &end_sys);

    const double user_time = end_user - start_user;
    const double sys_time = end_sys - start_sys;
    const double total_time = user_time + sys_time;
    printf("%.3f = %.3f + %.3f\n", total_time, user_time, sys_time);
    return EXIT_SUCCESS;
}

/*************************************************************************/
/*************************************************************************/
