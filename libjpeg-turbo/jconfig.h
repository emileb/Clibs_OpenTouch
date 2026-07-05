/* jconfig.h - hand-generated for the Android NDK build (no CMake).
 * 8-bit, arithmetic coding on, SIMD off (pure C via jsimd_none.c). */

#define JPEG_LIB_VERSION  62

#define LIBJPEG_TURBO_VERSION  2.1.5.1
#define LIBJPEG_TURBO_VERSION_NUMBER  2001005

#define C_ARITH_CODING_SUPPORTED 1
#define D_ARITH_CODING_SUPPORTED 1

/* In-memory source/destination managers (jpeg_mem_src used by q2repro). */
#define MEM_SRCDST_SUPPORTED  1

/* WITH_SIMD left undefined: pure-C build. */

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif

/* RIGHT_SHIFT_IS_UNSIGNED left undefined (bionic shifts signed correctly). */
