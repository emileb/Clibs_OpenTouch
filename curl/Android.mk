LOCAL_PATH := $(call my-dir)

# curl 8.22, HTTP/HTTPS only, OpenSSL 3.5. Built by ~/Android/GIT/android-curl-build/build.sh
# Needs openssl_ssl and openssl_crypto (../openssl) after it, and -lz

include $(CLEAR_VARS)

LOCAL_MODULE    := curl
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
LOCAL_EXPORT_LDLIBS := -lz

include $(PREBUILT_STATIC_LIBRARY)
