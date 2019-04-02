
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := logwritter

LOCAL_CFLAGS :=   -O2

LOCAL_C_INCLUDES := .

LOCAL_SRC_FILES =  LogWritter.c fts.c

LOCAL_LDLIBS :=  -ldl -llog

include $(BUILD_STATIC_LIBRARY)


