#if defined(__i386__) || defined(__amd64__) || defined(__x86_64__) \
 || defined(_M_X86) || defined(_M_X64) || defined(__arm__) || defined(_M_ARM) \
 || defined(__aarch64__)
# undef BIG_ENDI
# define LITTLE_ENDI 1
# define HIGH_HALF 1
# define  LOW_HALF 0
#else  /* Assume everything else is big-endian. */
# define BIG_ENDI 1
# undef LITTLE_ENDI
# define HIGH_HALF 0
# define  LOW_HALF 1
#endif
