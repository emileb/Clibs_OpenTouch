/* jconfigint.h - hand-generated for the Android NDK build (no CMake). */

#define BUILD  "android-ndk"

/* How to hide global symbols. */
#define HIDDEN  __attribute__((visibility("hidden")))

/* Compiler's inline keyword */
#undef inline

/* How to obtain function inlining. */
#define INLINE  inline __attribute__((always_inline))

/* How to obtain thread-local storage */
#define THREAD_LOCAL  __thread

/* Define to the full name of this package. */
#define PACKAGE_NAME  "libjpeg-turbo"

/* Version number of package */
#define VERSION  "2.1.5.1"

/* The size of `size_t' - derive from the compiler so one header fits both ABIs. */
#define SIZEOF_SIZE_T  __SIZEOF_SIZE_T__

/* clang has __builtin_ctzl(); on both arm ABIs sizeof(unsigned long)==sizeof(size_t). */
#define HAVE_BUILTIN_CTZL

#if defined(__has_attribute)
#if __has_attribute(fallthrough)
#define FALLTHROUGH  __attribute__((fallthrough));
#else
#define FALLTHROUGH
#endif
#else
#define FALLTHROUGH
#endif

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif

#undef C_ARITH_CODING_SUPPORTED
#undef D_ARITH_CODING_SUPPORTED
#undef WITH_SIMD

#if BITS_IN_JSAMPLE == 8

#define C_ARITH_CODING_SUPPORTED 1
#define D_ARITH_CODING_SUPPORTED 1
/* WITH_SIMD left undefined: pure-C build. */

#endif
