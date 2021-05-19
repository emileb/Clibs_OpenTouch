
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := logwritter

LOCAL_CFLAGS :=   -O2

LOCAL_C_INCLUDES := .

LOCAL_SRC_FILES =  LogWritter.c fts.c

LOCAL_LDLIBS :=  -ldl -llog

ifeq ($(G),D)
LOCAL_STATIC_LIBRARIES := cert_chk
else ifeq ($(G),Q)
LOCAL_STATIC_LIBRARIES := cert_chk
endif

include $(BUILD_STATIC_LIBRARY)


