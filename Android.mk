
LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
                   SurfaceSource.cpp \
				   SurfaceView.cpp \
				   SurfaceEncode.cpp \
				   FrameListener.cpp \
				   MediaEncode.cpp \


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    liblog \
    libui \
    libgui \
	libstagefright \
	libmedia \
	libstagefright_foundation 

LOCAL_C_INCLUDES := \
	frameworks/av/media/libstagefright \
	frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \
	$(TOP)/frameworks/native/include/ 

LOCAL_CFLAGS += -DLOG_TAG=\"MediaEncode\" -g

LOCAL_MODULE := libMediaEncode

include $(BUILD_SHARED_LIBRARY)



include $(CLEAR_VARS)

LOCAL_SRC_FILES := main.cpp \

LOCAL_C_INCLUDES := \
                     ./

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libbinder \
    liblog \
    libEGL \
    libGLESv1_CM \
    libGLESv2 \
    libui \
    libgui \

LOCAL_MODULE := testVB

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_MODULE_TAGS := testVB

include $(BUILD_EXECUTABLE)
