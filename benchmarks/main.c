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
extern int dhry_noopt_main(int count);
extern int dhry_opt_main(int count);

/*-----------------------------------------------------------------------*/

/* List of and data for available benchmarks. */

typedef struct Benchmark {
    const char *id;     // Identifier for use in the command line.
    const char *name;   // Human-readable name.
    /* Entry point for native benchmarking.  Takes an iteration count
     * and returns success (nonzero) or failure (zero). */
    int (*native_entry)(int count);
    /* Guest code blobs  for each guest architecture, using the same
     * function signature for the entry point. */
    const Blob *guest_code[GUEST_ARCH__NUM];
} Benchmark;

static const Benchmark benchmarks[] = {
    {.id = "dhry_noopt", .name = "Dhrystone (unoptimized)",
     .native_entry = dhry_noopt_main, .guest_code = {
            [GUEST_ARCH_PPC_7XX] = &ppc32_dhry_noopt,
        },
    },
    {.id = "dhry_opt", .name = "Dhrystone (optimized)",
     .native_entry = dhry_opt_main, .guest_code = {
            [GUEST_ARCH_PPC_7XX] = &ppc32_dhry_opt,
        },
    },
};

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
    for (int arch = 0; arch < GUEST_ARCH__NUM; arch++) {
        if (strcmp(name, arch_names[arch]) == 0) {
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
 * process_command_line:  Process the command line arguments and return
 * the selected architecture, benchmark, and iteration count.
 *
 * [Parameters]
 *     argc: Command line argument count.
 *     argv: Command line argument vector.
 *     arch_ret: Pointer to variable to receive the selected guest
 *         architecture.
 *     benchmark_ret: Pointer to variable to receive a pointer to the
 *         Benchmark structure for the selected benchmark.
 *     count_ret: Pointer to variable to receive the iteration count.
 * [Return value]
 *     True on success, false on error.
 */
static bool process_command_line(int argc, char **argv, GuestArch *arch_ret,
                                 const Benchmark **benchmark_ret,
                                 int *count_ret)
{
    if (argc > 1 && (strcmp(argv[1], "help") == 0
                     || strcmp(argv[1], "-h") == 0
                     || strncmp(argv[1], "--help", strlen(argv[1])) == 0)) {
        fprintf(stderr, "Usage: %s ARCH-NAME BENCHMARK ITERATION-COUNT\n",
                argv[0]);
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
                "If the benchmark does not complete successfully, nothing is printed.\n");
        return false;
    }

    if (argc != 4) {
        fprintf(stderr, "Wrong number of arguments\n");
      usage:
        fprintf(stderr, "Usage: %s ARCH-NAME BENCHMARK ITERATION-COUNT\n",
                argv[0]);
        fprintf(stderr, "Use %s -h for help.\n", argv[0]);
        return false;
    }

    if (strcmp(argv[1], "native") == 0) {
        *arch_ret = GUEST_ARCH_NATIVE;
    } else {
        *arch_ret = find_guest_arch(argv[1]);
        if (*arch_ret == GUEST_ARCH_INVALID) {
            fprintf(stderr, "Unknown architecture name: %s\n", argv[1]);
            goto usage;
        }
    }

    *benchmark_ret = find_benchmark(argv[2]);
    if (!*benchmark_ret) {
        fprintf(stderr, "Unknown benchmark: %s\n", argv[2]);
        goto usage;
    }

    char *eos;
    *count_ret = (int)strtol(argv[3], &eos, 0);
    if (*eos || *count_ret <= 0) {
        fprintf(stderr, "Invalid iteration count: %s\n", argv[3]);
        goto usage;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * call_guest:  Call the entry point for the selected guest architecture
 * and benchmark, and return its result.
 *
 * [Parameters]
 *     arch: Guest architecture.
 *     benchmark: Pointer to Benchmark structure for the benchmark to run.
 *     count: Iteration count.
 * [Return value]
 *     True if the benchmark completed successfully, false on error.
 */
static bool call_guest(GuestArch arch, const Benchmark *benchmark, int count)
{
    binrec_arch_t binrec_arch;
    void *state;
    uint32_t *retval_ptr;

    const Blob *blob = benchmark->guest_code[arch];
    if (!blob) {
        fprintf(stderr, "Blob missing for benchmark %s, architecture %s\n",
                benchmark->id, arch_names[arch]);
        return false;
    }
    void *memory = malloc(blob->reserve);
    if (!memory) {
        fprintf(stderr, "Failed to reserve guest memory (0x%X bytes)\n",
                blob->reserve);
    }
    memcpy((char *)memory + blob->base, blob->data, blob->size);

    switch (arch) {
      case GUEST_ARCH_PPC_7XX: {
        static PPCState ppc_state;
        memset(&ppc_state, 0, sizeof(ppc_state));
        ppc_state.gpr[1] = blob->base;
        ppc_state.gpr[3] = count;
        binrec_arch = BINREC_ARCH_PPC_7XX;
        state = &ppc_state;
        retval_ptr = &ppc_state.gpr[3];
        break;
      }
      default:
        fprintf(stderr, "Invalid guest architecture %d\n", arch);
        return false;
    }

    if (!call_guest_code(binrec_arch, state, memory, blob->base + blob->entry,
                         NULL)) {
        fprintf(stderr, "Guest code execution failed\n");
        free(memory);
        return false;
    }

    free(memory);
    return *retval_ptr != 0;
}

/*************************************************************************/
/************************** Program entry point **************************/
/*************************************************************************/

int main(int argc, char **argv)
{
    GuestArch arch;
    const Benchmark *benchmark;
    int count;
    if (!process_command_line(argc, argv, &arch, &benchmark, &count)) {
        return EXIT_FAILURE;
    }

    double start_user, start_sys;
    get_cpu_time(&start_user, &start_sys);

    int result;
    if (arch == GUEST_ARCH_NATIVE) {
        result = (*benchmark->native_entry)(count);
    } else {
        result = call_guest(arch, benchmark, count);
    }
    if (!result) {
        fprintf(stderr, "Benchmark reported failure\n");
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
