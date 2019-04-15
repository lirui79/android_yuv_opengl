#ifndef SURFACE_ENCODER_H
#define SURFACE_ENCODER_H


#include <media/stagefright/MediaCodec.h>
#include <gui/Surface.h>  
#include <thread>
#include "FrameListener.h"

using namespace android;

class SurfaceEncode {
public:
    SurfaceEncode() ;
    virtual ~SurfaceEncode();

    int onCreate(int width, int height, int yuv=1) ;

    int start();

    sp<IGraphicBufferProducer> getIGraphicBufferProducer() const;
protected:
    int onCreateSurface(int width, int height) ;
    int onCreateYuv(int width, int height) ;
    int encode_input_thread();
    int encode_output_thread();
private:
    sp<MediaCodec> encoder;
    sp<IGraphicBufferProducer> inputProducer;
    sp<IGraphicBufferConsumer> inputConsumer;
    std::list<std::thread*>    mThreads;
    sp<FrameListener>  mFrameListener;
    int width;
    int height;
    int yuv;
};

#endif //SURFACE_ENCODER_H