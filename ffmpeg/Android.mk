# Prebuilt FFmpeg static libs (per ABI) for q2repro cinematic playback (cin.c)
# and ogg music (ogg.c). Built PIC (--enable-pic) so the .a link into the shared
# libq2repro.so. Smaller than shipping the 5 .so. Inter-lib deps are declared so
# ndk-build orders the link correctly; avutil exports libm/libz.
# Minimal build: theora/vorbis/idcin/pcm decoders, ogg/idcin/wav demuxers.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg-avutil
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libavutil.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
LOCAL_EXPORT_LDLIBS := -lm -lz
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg-swresample
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libswresample.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
LOCAL_STATIC_LIBRARIES := ffmpeg-avutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg-swscale
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libswscale.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
LOCAL_STATIC_LIBRARIES := ffmpeg-avutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg-avcodec
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libavcodec.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
LOCAL_STATIC_LIBRARIES := ffmpeg-swresample ffmpeg-avutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg-avformat
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libavformat.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
LOCAL_STATIC_LIBRARIES := ffmpeg-avcodec ffmpeg-swscale ffmpeg-swresample ffmpeg-avutil
include $(PREBUILT_STATIC_LIBRARY)
