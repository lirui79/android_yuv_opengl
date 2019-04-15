#ifndef SURFACE_SOURCE_H
#define SURFACE_SOURCE_H

#include <list>
#include <vector>
#include <thread>
#include <gui/BufferItem.h>
#include <gui/BufferQueue.h>
#include <gui/Surface.h>
#include <gui/IConsumerListener.h>
#include <gui/ISurfaceComposer.h>
#include <private/gui/ComposerService.h>
#include <gui/CpuConsumer.h>
#include <media/stagefright/MediaCodec.h>
#include "FrameListener.h"

using namespace android;

class SurfaceSource {
private:
    class FrameAvailableListener : public ConsumerBase::FrameAvailableListener {
        public:
        FrameAvailableListener() : mPendingFrames(0) {
        }

        virtual ~FrameAvailableListener() {
        }

        void waitForFrame() {
            Mutex::Autolock lock(mMutex);
            if (mPendingFrames <= 0) {
                mCondition.wait(mMutex);
            }
            --mPendingFrames;
        }

        void onFrameAvailable(const BufferItem& /*item*/) {
            Mutex::Autolock lock(mMutex);
            ++mPendingFrames;
            mCondition.signal();
        }

        private:
            int mPendingFrames;
            Mutex mMutex;
            Condition mCondition;
    };

    typedef struct BufferInfo {
        uint8_t    *data;
        uint32_t    size;
        uint32_t    width;
        uint32_t    height;
        PixelFormat format;
        uint32_t    stride;
        Rect        crop;
        uint32_t    transform;
        uint32_t    scalingMode;
        int64_t     timestamp;
        android_dataspace dataSpace;
        uint64_t    frameNumber;
    } BufferInfo;

public:
    SurfaceSource();
    virtual ~SurfaceSource();

    int onCreate(int w , int h, int yuv = 0) ;

    int setData(const char *fileName, int width, int height) ;

    int onClose();

    int addOutput(sp<IGraphicBufferProducer> outputProducer) ;

    void* getNativeWindow();

    int  start();

protected:
    int  getYUV12Data(FILE *fp, unsigned char * pYUVData,int size, int nY4M);
    void render(const void *data, size_t size,int width,int height);
    void setdata(sp<IGraphicBufferProducer> inputgbp, android_dataspace dataspace, const void *data, size_t size,int width,int height);
    void setdata(sp<IGraphicBufferProducer> inputgbp, struct BufferInfo *buffer);
    int  setRGBAData(int width, int height);
    int  connect();
    int  consumer_thread();
    int  StreamSplit(unsigned char *buffer, int sz);
    int  Splitter();
    int  encode_input_thread();
    int  encode_output_thread();
    int  setBuffer(const void *data, size_t size, int width, int height, int64_t timestamp, int yuv);
    int  RGBA_To_YUV420SP(const unsigned char *rgba, int width, int height, unsigned char* yuv420sp);
private:
    sp<MediaCodec>      mEncoder;
    std::list<size_t>   mInputBuffers;
    Mutex               mMutex;

    sp<IGraphicBufferProducer> inputProducer;
    sp<IGraphicBufferConsumer> inputConsumer;
    sp<CpuConsumer>            mConsumer;
    std::vector<sp<IGraphicBufferProducer> > mOutputs;
    std::list<std::thread*>    mThreads;
    sp<FrameAvailableListener>  mFrameAvailableListener;
    sp<Surface>  surface;
    sp<ANativeWindow> window;
    int width ;
    int height ;
    int yuv ;
};

#endif //SURFACE_SOURCE_H