# libjpeg-turbo 2.1.5.1 static lib for the Android NDK (no CMake).
# Pure-C build (SIMD off via jsimd_none.c). Provides JCS_EXT_RGBA, which the
# classic jpeg8d lacks and q2repro's images.c needs. Module name 'jpegturbo'
# is distinct from jpeg8d's 'libjpeg' so both can coexist in the tree.
# Config headers (jconfig.h/jconfigint.h/jversion.h) are hand-generated here.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := jpegturbo

# JPEG_SOURCES from CMakeLists.txt (8-bit) + arith + jsimd_none (no SIMD).
LOCAL_SRC_FILES := \
	jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
	jcicc.c jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
	jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdatadst.c \
	jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdicc.c jdinput.c \
	jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c \
	jdtrans.c jerror.c jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c \
	jidctint.c jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c \
	jaricom.c jcarith.c jdarith.c \
	jsimd_none.c

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS += -O2 -fvisibility=hidden -fdata-sections -ffunction-sections

include $(BUILD_STATIC_LIBRARY)
