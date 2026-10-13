LOCAL_PATH := $(call my-dir)

# OpenSSL 3.5 LTS, static. Built by ~/Android/GIT/android-curl-build/build.sh
# Link order: curl openssl_ssl openssl_crypto

include $(CLEAR_VARS)
LOCAL_MODULE := openssl_ssl
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := openssl_crypto
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)
