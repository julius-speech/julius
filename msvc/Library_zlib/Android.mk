LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := zlib
LOCAL_SRC_FILES  := src/adler32.c \
                    src/compress.c \
                    src/crc32.c \
                    src/contrib/minizip/ioapi.c \
                    src/contrib/minizip/unzip.c \
                    src/deflate.c \
                    src/gzclose.c \
                    src/gzlib.c \
                    src/gzread.c \
                    src/gzwrite.c \
                    src/infback.c \
                    src/inffast.c \
                    src/inflate.c \
                    src/inftrees.c \
                    src/trees.c \
                    src/uncompr.c \
                    src/zutil.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS     += -DIOAPI_NO_64

include $(BUILD_STATIC_LIBRARY)
