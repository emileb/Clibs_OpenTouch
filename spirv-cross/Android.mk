# SPIRV-Cross (KhronosGroup/SPIRV-Cross 9c3c8e2c, 2026-07-31), GLSL backend via the C API only.
# One shared lib for all GZDoom-family engines; the C API keeps C++ exceptions inside this .so.
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := spirvcross

LOCAL_CPPFLAGS := -std=c++17 -fexceptions -fvisibility=hidden -DSPIRV_CROSS_C_API_GLSL=1 -DSPVC_EXPORT_SYMBOLS

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES = \
	spirv_cross.cpp \
	spirv_parser.cpp \
	spirv_cross_parsed_ir.cpp \
	spirv_cfg.cpp \
	spirv_glsl.cpp \
	spirv_cross_c.cpp

include $(BUILD_SHARED_LIBRARY)
