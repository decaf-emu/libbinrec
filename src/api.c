/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/bitutils.h"
#include "src/common.h"
#include "src/guest-ppc.h"
#include "src/host-x86.h"
#include "src/rtl.h"

/* Disable malloc() suppression from common.h. */
#undef malloc
#undef realloc
#undef free

#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/*************************** Local data types ****************************/
/*************************************************************************/

/* Function types for guest and host translators. */
typedef bool GuestTranslateFunc(binrec_t *handle, uint32_t address,
                                uint32_t limit, RTLUnit *unit);
typedef bool HostTranslateFunc(binrec_t *handle, RTLUnit *unit);

/*************************************************************************/
/*************************** Helper functions ****************************/
/*************************************************************************/

/**
 * arch_name:  Return a human-readable name for the given architecture.
 */
static const char *arch_name(binrec_arch_t arch)
{
    switch (arch) {
      case BINREC_ARCH_PPC_7XX:
        return "PowerPC 6xx/7xx";
      case BINREC_ARCH_X86_64_SYSV:
        return "x86-64 (SysV ABI)";
      case BINREC_ARCH_X86_64_WINDOWS:
        return "x86-64 (Windows ABI)";
      case BINREC_ARCH_X86_64_WINDOWS_SEH:
        return "x86-64 (Windows ABI with unwind data)";
      case BINREC_ARCH_INVALID:  // Avoid a compiler warning.
        break;
    }
    return "(invalid architecture)";
}

/*-----------------------------------------------------------------------*/

/**
 * arch_little_endian:  Return whether the given architecture is little-endian.
 */
static bool arch_is_little_endian(binrec_arch_t arch)
{
    switch (arch) {
      case BINREC_ARCH_INVALID:  // Avoid a compiler warning.
      case BINREC_ARCH_PPC_7XX:
        return false;
      case BINREC_ARCH_X86_64_SYSV:
      case BINREC_ARCH_X86_64_WINDOWS:
      case BINREC_ARCH_X86_64_WINDOWS_SEH:
        return true;
    }
    return false;
}

/*-----------------------------------------------------------------------*/

/**
 * arch_guest_translate_func:  Return the guest (input) translation
 * function for the given architecture, or NULL if the architecture is not
 * supported as a guest.
 */
static GuestTranslateFunc *arch_guest_translate_func(binrec_arch_t arch)
{
    switch (arch) {
      case BINREC_ARCH_PPC_7XX:
        return guest_ppc_translate;
      default:
        return NULL;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * arch_host_translate_func:  Return the host (input) translation
 * function for the given architecture, or NULL if the architecture is not
 * supported as a host.
 */
static HostTranslateFunc *arch_host_translate_func(binrec_arch_t arch)
{
    switch (arch) {
      case BINREC_ARCH_X86_64_SYSV:
      case BINREC_ARCH_X86_64_WINDOWS:
      case BINREC_ARCH_X86_64_WINDOWS_SEH:
        return host_x86_translate;
      default:
        return NULL;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * arch_host_code_alignment:  Return the preferred alignment for generated
 * host code for the given architecture, or 0 if the architecture is not
 * supported as a host.
 */
static int arch_host_code_alignment(binrec_arch_t arch)
{
    switch (arch) {
      case BINREC_ARCH_X86_64_SYSV:
      case BINREC_ARCH_X86_64_WINDOWS:
      case BINREC_ARCH_X86_64_WINDOWS_SEH:
        return 16;
      default:
        return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * lookup_partial_readonly_page:  Look up the given page in the partial
 * readonly page tables.  If the page is present, return its table index;
 * otherwise, return the index at which it should be inserted (which may be
 * past the end of the table if the table is full).
 *
 * [Parameters]
 *     handle: Handle to use for lookup.
 *     page: Page number (address >> READONLY_PAGE_BITS).
 * [Return value]
 *     Index in handle->partial_readonly_* arrays of the page if it exists
 *     in the tables, otherwise the index at which it should be inserted.
 */
static int lookup_partial_readonly_page(const binrec_t *handle, uint32_t page)
{
    int low = 0, high = handle->num_partial_readonly - 1;
    while (low <= high) {
        const int mid = (low + high) / 2;
        if (page == handle->partial_readonly_pages[mid]) {
            return mid;
        } else if (page < handle->partial_readonly_pages[mid]) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return low;
}

/*-----------------------------------------------------------------------*/

/**
 * add_partial_readonly_page:  Mark the given partial page as read-only.
 * Helper for binrec_add_readonly_region().
 */
static int add_partial_readonly_page(binrec_t *handle,
                                     uint32_t start, uint32_t end)
{
    ASSERT(handle);
    ASSERT(end >= start);

    const uint32_t page = start >> READONLY_PAGE_BITS;

    const int index = lookup_partial_readonly_page(handle, page);
    if (index == MAX_PARTIAL_READONLY) {
        log_error(handle, "Failed to add read-only range [0x%X,0x%X]: no free"
                  " partial page entries", start, end-1);
        return 0;
    }

    if (page != handle->partial_readonly_pages[index]) {
        if (handle->partial_readonly_pages[MAX_PARTIAL_READONLY - 1] != ~0u) {
            log_error(handle, "Failed to add read-only range [0x%X,0x%X]: no"
                      " free partial page entries", start, end-1);
            return 0;
        }
        memmove(&handle->partial_readonly_pages[index],
                &handle->partial_readonly_pages[index+1],
                sizeof(*handle->partial_readonly_pages) * (handle->num_partial_readonly - index));
        memmove(&handle->partial_readonly_map[index],
                &handle->partial_readonly_map[index+1],
                sizeof(*handle->partial_readonly_map) * (handle->num_partial_readonly - index));
        handle->num_partial_readonly++;
        handle->partial_readonly_pages[index] = page;
        memset(handle->partial_readonly_map[index], 0,
               sizeof(*handle->partial_readonly_map));
        handle->readonly_map[page>>2] |= 1 << ((page & 3) * 2);
    }

    start -= page << READONLY_PAGE_BITS;
    end -= page << READONLY_PAGE_BITS;
    ASSERT(end <= READONLY_PAGE_SIZE);
    for (uint32_t address = start; address < end; address++) {
        handle->partial_readonly_map[index][address>>3] |= 1 << (address & 7);
    }
    return 1;
}

/*************************************************************************/
/********************** Internal utility functions ***********************/
/*************************************************************************/

bool is_address_readonly(const binrec_t *handle, uint32_t address)
{
    const uint32_t page = address >> READONLY_PAGE_BITS;
    if (handle->readonly_map[page>>2] & (2 << ((page & 3) * 2))) {
        return true;
    } else if (handle->readonly_map[page>>2] & (1 << ((page & 3) * 2))) {
        const int index = lookup_partial_readonly_page(handle, page);
        ASSERT(index < lenof(handle->partial_readonly_pages));
        ASSERT(handle->partial_readonly_pages[index] == page);
        const uint32_t page_offset = address & READONLY_PAGE_MASK;
        return ((handle->partial_readonly_map[index][page_offset>>3]
                 & (1 << (page_offset & 7))) != 0);
    } else {
        return false;
    }
}

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

const char *binrec_version(void)
{
    return VERSION;
}

/*-----------------------------------------------------------------------*/

binrec_arch_t binrec_native_arch(void)
{
    #if defined(_M_X64)
        return BINREC_ARCH_X86_64_WINDOWS;
    #elif defined(__amd64__) || defined(__x86_64__)
        return BINREC_ARCH_X86_64_SYSV;
    #else
        return BINREC_ARCH_INVALID;
    #endif
}

/*-----------------------------------------------------------------------*/

binrec_arch_t binrec_native_features(void)
{
    #if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)

        uint32_t ecx_1, ecx_80000001, ebx_7;

        #if defined(_MSC_VER)
            int output[4];
            __cpuid(output, 1);
            ecx_1 = output[2];
            __cpuid(output, 0x80000001);
            ecx_80000001 = output[2];
            __cpuidex(output, 7, 0);
            ebx_7 = output[1];
        #elif defined(__GNUC__)
            uint32_t dummy, dummy2;
            __asm__("cpuid" : "=c" (ecx_1) : "a" (1) : "ebx", "edx");
            __asm__("cpuid" : "=a" (dummy), "=c" (ecx_80000001)
                            : "0" (0x80000001) : "ebx", "edx");
            __asm__("cpuid" : "=a" (dummy), "=b" (ebx_7), "=c" (dummy2)
                            : "0" (7), "2" (0) : "edx");
        #else
            #warning No method to call CPUID, will always return 0
            ecx_1 = 0;
            ecx_80000001 = 0;
            ebx_7 = 0;
        #endif

        uint64_t features = 0;
        /* These macros allow setting feature bits without conditional
         * operations, so that coverage doesn't depend on finding many
         * CPUs that implement different sets of features.  This is
         * possibly a bit of overkill for the trivial purpose of improving
         * coverage, but it's not that bad in the grand scheme of things.
         * Also, it works just fine. */
        #define COPY_FEATURE_BIT_LEFT(src, bit, feature)  do { \
            STATIC_ASSERT((feature) >= 1<<(bit), "Wrong shift direction"); \
            features |= ((src) & (1<<(bit))) << (ctz32((feature)) - (bit)); \
        } while (0)
        #define COPY_FEATURE_BIT_RIGHT(src, bit, feature)  do { \
            STATIC_ASSERT((feature) <= 1<<(bit), "Wrong shift direction"); \
            features |= ((src) & (1<<(bit))) >> ((bit) - ctz32((feature))); \
        } while (0)
        COPY_FEATURE_BIT_RIGHT(ecx_1, 12, BINREC_FEATURE_X86_FMA);
        COPY_FEATURE_BIT_RIGHT(ecx_1, 22, BINREC_FEATURE_X86_MOVBE);
        COPY_FEATURE_BIT_RIGHT(ecx_80000001, 5, BINREC_FEATURE_X86_LZCNT);
        COPY_FEATURE_BIT_LEFT (ebx_7, 3, BINREC_FEATURE_X86_BMI1);
        COPY_FEATURE_BIT_RIGHT(ebx_7, 8, BINREC_FEATURE_X86_BMI2);
        return features;

    #else  // Unsupported architecture.
        return 0;
    #endif
}

/*-----------------------------------------------------------------------*/

int binrec_guest_supported(binrec_arch_t arch)
{
    return arch_guest_translate_func(arch) != NULL;
}

/*-----------------------------------------------------------------------*/

int binrec_host_supported(binrec_arch_t arch)
{
    return arch_host_translate_func(arch) != NULL;
}

/*-----------------------------------------------------------------------*/

binrec_t *binrec_create_handle(const binrec_setup_t *setup)
{
    ASSERT(setup);

    binrec_t *handle;
    if (setup->malloc) {
        handle = (*setup->malloc)(setup->userdata, sizeof(*handle));
    } else {
        handle = malloc(sizeof(*handle));
    }
    if (UNLIKELY(!handle)) {
        if (setup->log) {
            (*setup->log)(setup->userdata, BINREC_LOGLEVEL_ERROR,
                          "No memory for handle");
        }
        return NULL;
    }

    memset(handle, 0, sizeof(*handle));
    handle->setup = *setup;
    handle->host_little_endian = arch_is_little_endian(setup->host);
    handle->code_buffer = NULL;
    handle->pre_insn_callback = NULL;
    handle->post_insn_callback = NULL;

    const int have_malloc = (setup->malloc != NULL);
    const int have_realloc = (setup->realloc != NULL);
    const int have_free = (setup->free != NULL);
    if (have_malloc + have_realloc + have_free != 3
     && have_malloc + have_realloc + have_free > 0) {
        log_warning(handle, "Some but not all memory allocation functions"
                    " were defined.  Using system allocator instead.");
        handle->setup.malloc = NULL;
        handle->setup.realloc = NULL;
        handle->setup.free = NULL;
    }

    const int have_code_malloc = (setup->code_malloc != NULL);
    const int have_code_realloc = (setup->code_realloc != NULL);
    const int have_code_free = (setup->code_free != NULL);
    if (have_code_malloc + have_code_realloc + have_code_free != 3
     && have_code_malloc + have_code_realloc + have_code_free > 0) {
        log_warning(handle, "Some but not all output code memory allocation"
                    " functions were defined.  Using default allocator"
                    " instead.");
        handle->setup.code_malloc = NULL;
        handle->setup.code_realloc = NULL;
        handle->setup.code_free = NULL;
    }

    handle->code_range_end = -1;
    handle->max_inline_depth = 1;
    memset(handle->partial_readonly_pages, 0xFF,
           sizeof(handle->partial_readonly_pages));

    return handle;
}

/*-----------------------------------------------------------------------*/

void binrec_destroy_handle(binrec_t *handle)
{
    /* We should never have a leftover code buffer; either it will be
     * transferred to the caller of binrec_translate(), or it will be
     * freed by binrec_translate() due to translation failure. */
    ASSERT(!handle->code_buffer);

    binrec_free(handle, handle);
}

/*-----------------------------------------------------------------------*/

void binrec_set_code_range(binrec_t *handle, uint32_t start, uint32_t end)
{
    ASSERT(handle);

    if (UNLIKELY(end < start)) {
        log_error(handle, "Invalid code range 0x%X-0x%X", start, end);
        /* Signal an invalid state to binrec_translate(). */
        handle->code_range_start = 1;
        handle->code_range_end = 0;
        return;
    }

    handle->code_range_start = start;
    handle->code_range_end = end;
}

/*-----------------------------------------------------------------------*/

void binrec_set_optimization_flags(
    binrec_t *handle, unsigned int common_opt, unsigned int guest_opt,
    unsigned int host_opt)
{
    ASSERT(handle);

    handle->common_opt = common_opt;
    handle->guest_opt = guest_opt;
    handle->host_opt = host_opt;
}

/*-----------------------------------------------------------------------*/

void binrec_set_max_inline_length(binrec_t *handle, int length)
{
    ASSERT(handle);

    if (length >= 0) {
        handle->max_inline_length = length;
    } else {
        log_error(handle, "Invalid maximum inline length %d", length);
    }
}

/*-----------------------------------------------------------------------*/

void binrec_set_max_inline_depth(binrec_t *handle, int depth)
{
    ASSERT(handle);

    if (depth >= 0) {
        handle->max_inline_depth = depth;
    } else {
        log_error(handle, "Invalid maximum inline depth %d", depth);
    }
}

/*-----------------------------------------------------------------------*/

int binrec_add_readonly_region(binrec_t *handle, uint32_t base, uint32_t size)
{
    ASSERT(handle);

    uint32_t top = base + size;

    if ((base & READONLY_PAGE_MASK) != 0) {
        const uint32_t page_top =
            min((base & ~READONLY_PAGE_MASK) + READONLY_PAGE_SIZE, top);
        if (!add_partial_readonly_page(handle, base, page_top)) {
            return 0;
        }
        base = page_top;
        if (base == top) {
            return 1;
        }
    }

    if ((top & READONLY_PAGE_MASK) != 0) {
        const uint32_t page_base = top & ~READONLY_PAGE_MASK;
        if (!add_partial_readonly_page(handle, page_base, top)) {
            return 0;
        }
    }

    base >>= READONLY_PAGE_BITS;
    top >>= READONLY_PAGE_BITS;
    for (uint32_t page = base; page < top; page++) {
        handle->readonly_map[page>>2] &= ~(1 << ((page & 3) * 2));
        handle->readonly_map[page>>2] |= 2 << ((page & 3) * 2);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/

void binrec_clear_readonly_regions(binrec_t *handle)
{
    ASSERT(handle);

    memset(handle->readonly_map, 0, sizeof(handle->readonly_map));
    memset(handle->partial_readonly_pages, 0xFF,
           sizeof(handle->partial_readonly_pages));
    handle->num_partial_readonly = 0;
}

/*-----------------------------------------------------------------------*/

void binrec_enable_chaining(binrec_t *handle, int enable)
{
    ASSERT(handle);

    if (enable && handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH) {
        log_warning(handle, "Dynamic chaining is not possible with the"
                    " Windows SEH ABI");
        return;
    } else {
        handle->use_chaining = (enable != 0);
    }
}

/*-----------------------------------------------------------------------*/

void binrec_enable_branch_exit_test(binrec_t *handle, int enable)
{
    ASSERT(handle);
    handle->use_branch_exit_test = (enable != 0);
}

/*-----------------------------------------------------------------------*/

void binrec_set_pre_insn_callback(binrec_t *handle,
                                  void (*callback)(void *, uint32_t))
{
    ASSERT(handle);
    handle->pre_insn_callback = callback;
}

/*-----------------------------------------------------------------------*/

void binrec_set_post_insn_callback(binrec_t *handle,
                                   void (*callback)(void *, uint32_t))
{
    ASSERT(handle);
    handle->post_insn_callback = callback;
}

/*-----------------------------------------------------------------------*/

void binrec_enable_verify(binrec_t *handle, int enable)
{
    ASSERT(handle);
    handle->do_verify = (enable != 0);
}

/*-----------------------------------------------------------------------*/

int binrec_translate(binrec_t *handle, void *state, uint32_t address,
                     uint32_t limit, void **code_ret, long *size_ret)
{
    ASSERT(handle);
    ASSERT(code_ret);
    ASSERT(size_ret);

    GuestTranslateFunc * const guest_translate =
        arch_guest_translate_func(handle->setup.guest);
    if (!guest_translate) {
        log_error(handle, "Unsupported guest architecture: %s",
                  arch_name(handle->setup.guest));
        return 0;
    }

    HostTranslateFunc * const host_translate =
        arch_host_translate_func(handle->setup.host);
    handle->code_alignment = arch_host_code_alignment(handle->setup.host);
    if (!host_translate) {
        log_error(handle, "Unsupported host architecture: %s",
                  arch_name(handle->setup.host));
        return 0;
    }

    if (UNLIKELY(handle->code_range_end < handle->code_range_start)) {
        log_error(handle, "Code range invalid");
        return 0;
    }
    if (UNLIKELY(address < handle->code_range_start)
     || UNLIKELY(address > handle->code_range_end)) {
        log_error(handle, "Address 0x%X not within code range 0x%X-0x%X",
                  address, handle->code_range_start, handle->code_range_end);
        return 0;
    }
    if (UNLIKELY(limit < address)) {
        log_error(handle, "Invalid translation range 0x%X-0x%X",
                  address, limit);
        return 0;
    }

    handle->opt_state = state;

    RTLUnit *unit = rtl_create_unit(handle);
    if (UNLIKELY(!unit)) {
        log_error(handle, "Failed to create RTLUnit");
        return 0;
    }

    if (!(*guest_translate)(handle, address, limit, unit)) {
        log_error(handle, "Failed to parse guest instructions starting at"
                  " 0x%X", address);
        rtl_destroy_unit(unit);
        return 0;
    }

    if (!rtl_finalize_unit(unit)) {
        log_error(handle, "Failed to finalize RTL for code at 0x%X", address);
        rtl_destroy_unit(unit);
        return 0;
    }

    if (!rtl_optimize_unit(unit, handle->common_opt)) {
        log_warning(handle, "Failed to optimize RTL for code at 0x%X",
                    address);
        /* Don't treat this as an error; just translate the unoptimized
         * unit. */
    }

    if (handle->do_verify) {
        if (!rtl_verify_unit(unit, address)) {
            log_error(handle, "RTL verification failed for code at 0x%X",
                      address);
            rtl_destroy_unit(unit);
            return 0;
        }
    }

    handle->code_buffer_size = CODE_EXPAND_SIZE;
    handle->code_buffer = binrec_code_malloc(
        handle, handle->code_buffer_size, handle->code_alignment);
    if (UNLIKELY(!handle->code_buffer)) {
        log_error(handle, "No memory for initial output code buffer (%ld"
                  " bytes)", handle->code_buffer_size);
        rtl_destroy_unit(unit);
        return 0;
    }
    handle->code_len = 0;

    const bool result = (*host_translate)(handle, unit);
    rtl_destroy_unit(unit);
    if (!result) {
        log_error(handle, "Failed to generate host code for 0x%X", address);
        binrec_code_free(handle, handle->code_buffer);
        handle->code_buffer = NULL;
        return 0;
    }

    void *shrunk_buffer = binrec_code_realloc(
        handle, handle->code_buffer, (size_t)handle->code_buffer_size,
        (size_t)handle->code_len, handle->code_alignment);
    if (LIKELY(shrunk_buffer)) {  // Should never fail, but play it safe.
        handle->code_buffer = shrunk_buffer;
    }

    *code_ret = handle->code_buffer;
    *size_ret = handle->code_len;
    handle->code_buffer = NULL;
    return 1;
}

/*************************************************************************/
/*************************************************************************/
