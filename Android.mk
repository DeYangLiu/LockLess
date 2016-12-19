LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := lazy
LOCAL_CFLAGS :=  #-finput-charset=gbk -fexec-charset=gbk

#LOCAL_CFLAGS += -DDEBUG
#LOCAL_CFLAGS += -g -O0  -fno-omit-frame-pointer

LOCAL_C_INCLUDES += external/jpeg  

LOCAL_SRC_FILES := LinkedListLazy.c febr.c TestHarness.c 
LOCAL_LDLIBS :=

LOCAL_SHARED_LIBRARIES :=  libcutils

include $(BUILD_EXECUTABLE)

############
include $(CLEAR_VARS)
LOCAL_MODULE := fine
LOCAL_SHARED_LIBRARIES :=  libcutils

LOCAL_SRC_FILES := LinkedListFine.c TestHarness.c

include $(BUILD_EXECUTABLE)
