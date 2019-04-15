#include "SurfaceSource.h"
#include <gui/IProducerListener.h>
#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/PersistentSurface.h>
#include <media/stagefright/MediaMuxer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/ICrypto.h>
#include <string.h>

#include <utils/Log.h>

SurfaceSource::SurfaceSource() : yuv(0) {        
}

SurfaceSource::~SurfaceSource() {
}

int SurfaceSource::onCreate(int w , int h, int y) {
    width  = w;
    height = h;
    yuv = y;
    BufferQueue::createBufferQueue(&inputProducer, &inputConsumer);
    if (yuv == 1) {
       inputConsumer->setDefaultBufferFormat(HAL_PIXEL_FORMAT_YV12);// ?HAL_PIXEL_FORMAT_YV12
    } else {
       inputConsumer->setDefaultBufferFormat(HAL_PIXEL_FORMAT_RGBA_8888);
    }
    inputConsumer->setDefaultBufferSize(width, height);
    
    mConsumer = new CpuConsumer(inputConsumer, 1, false);
    mConsumer->setName(android::String8("Surface Source"));
 //   mFrameListener = new FrameListener();
 //   sp<IConsumerListener> proxy = new BufferQueue::ProxyConsumerListener(mFrameListener);
   // inputConsumer->consumerConnect(proxy, false);
    mFrameAvailableListener = new FrameAvailableListener();
    mConsumer->setFrameAvailableListener(mFrameAvailableListener);

    std::thread* thread = new std::thread(&SurfaceSource::consumer_thread, this);
    mThreads.push_back(thread);
    return 0;
}

int SurfaceSource::connect() {
    IGraphicBufferProducer::QueueBufferOutput qbo;
    status_t err = inputProducer->connect(new DummyProducerListener, NATIVE_WINDOW_API_CPU, false,
            &qbo);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to IGraphicBufferProducer::connect (err=%d)\n", err);
        return -1;
    }
    return 0;  
}

int SurfaceSource::getYUV12Data(FILE *fp, unsigned char * pYUVData,int size, int nY4M) {
    int sz = fread(pYUVData,size,1,fp);
    if (nY4M != 0) {
         unsigned char temp[6] = "12345" ;
         fread(temp,6,1,fp);
    }
    if (sz > 0)
       return 0;
    return -1;
}

void* SurfaceSource::getNativeWindow() {
    if (surface == NULL) {
        surface = new Surface(inputProducer, false);
        window = surface;
    }
    return window.get();
}


void SurfaceSource::render(const void *data, size_t size,int width,int height) {
    int slot = -1;
    sp<Fence> fence;
    sp<GraphicBuffer> buffer;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_VIDEO_ENCODER; //| GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    inputProducer->dequeueBuffer(&slot, &fence, width, height, HAL_PIXEL_FORMAT_YV12, usage);//HAL_PIXEL_FORMAT_YV12

    inputProducer->requestBuffer(slot, &buffer);

    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
    Rect rect(0, 0, width, height);// HAL_DATASPACE_ARBITRARY  HAL_DATASPACE_UNKNOWN HAL_DATASPACE_V0_BT709 HAL_DATASPACE_V0_JFIF
    IGraphicBufferProducer::QueueBufferInput input = IGraphicBufferProducer::QueueBufferInput(nowTime, false, HAL_DATASPACE_UNKNOWN, rect,
              NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, Fence::NO_FENCE);
    IGraphicBufferProducer::QueueBufferOutput output;
   
    void *dataIn = NULL ;
    buffer->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, reinterpret_cast<void**>(&dataIn));
    memcpy(dataIn, data, size);//将yuv数据copy到图形缓冲区  
    buffer->unlock();
    inputProducer->queueBuffer(slot, input, &output);
}

void SurfaceSource::setdata(sp<IGraphicBufferProducer> inputgbp, android_dataspace dataspace, const void *data, size_t size,int width,int height) {
    int slot = -1;
    sp<Fence> fence;
    sp<GraphicBuffer> buffer;

    if (yuv == 0) {
        int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;  
        inputgbp->dequeueBuffer(&slot, &fence, width, height, HAL_PIXEL_FORMAT_RGBA_8888, usage);
    } else {
        int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_VIDEO_ENCODER; 
        inputgbp->dequeueBuffer(&slot, &fence, width, height, HAL_PIXEL_FORMAT_YV12, usage);// HAL_PIXEL_FORMAT_YV12 HAL_PIXEL_FORMAT_YCbCr_420_888
    }

    inputgbp->requestBuffer(slot, &buffer);

    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
    Rect rect(0, 0, width, height);// HAL_DATASPACE_ARBITRARY  HAL_DATASPACE_UNKNOWN
    IGraphicBufferProducer::QueueBufferInput input = IGraphicBufferProducer::QueueBufferInput(nowTime, false, dataspace, rect,
              NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, fence);
    IGraphicBufferProducer::QueueBufferOutput output;
   
    void *dataIn = NULL ;
    buffer->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, &dataIn);
    memcpy(dataIn, data, size);//将RGBA copy到图形缓冲区  
    buffer->unlock();
    inputgbp->queueBuffer(slot, input, &output);
}  

void SurfaceSource::setdata(sp<IGraphicBufferProducer> inputgbp, struct BufferInfo *buf) {
    int slot = -1;
    sp<Fence> fence;
    sp<GraphicBuffer> buffer;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_VIDEO_ENCODER; 
    inputgbp->dequeueBuffer(&slot, &fence, buf->width, buf->height, buf->format, usage);

    inputgbp->requestBuffer(slot, &buffer);

    Rect rect(0, 0, buf->width, buf->height);// HAL_DATASPACE_ARBITRARY  HAL_DATASPACE_UNKNOWN
    IGraphicBufferProducer::QueueBufferInput input = IGraphicBufferProducer::QueueBufferInput(buf->timestamp, false, buf->dataSpace, rect,
              NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, fence);
    IGraphicBufferProducer::QueueBufferOutput output;
   
    void *dataIn = NULL ;
    buffer->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, &dataIn);
    memcpy(dataIn, buf->data, buf->size);//将RGBA copy到图形缓冲区  
    buffer->unlock();
    inputgbp->queueBuffer(slot, input, &output);
    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
    ALOGI("SurfaceSource::setdata %lld  %lld %lld", nowTime, buf->timestamp, (nowTime - buf->timestamp)/1000000);

}  


int SurfaceSource::setRGBAData(int width, int height) {
    int size = width * height ;
    unsigned char *data = new unsigned char[4 * size];  
    int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000, curTime, diff;
    for (int i = 0 ; i < 10000000 ; ++i) {
        memset(data , 0  , 4 * size);
        for (int j = 0 ; j < size; ++j) {
           data[4 * j + (i%3)] = 255;
           data[4 * j + 3] = 255;
        }
        setdata(inputProducer, HAL_DATASPACE_UNKNOWN, data, 4 * size, width, height);
        curTime = systemTime(CLOCK_MONOTONIC) / 1000;
        diff = curTime - nowTime;
        ALOGI("timestamp %lX %lX render\n", curTime / 1000, diff / 1000);
        if (diff < 16000)
           usleep(16000 - diff);
        nowTime = curTime;
    }
    
    delete[]  data;
    return 0;
}

int SurfaceSource::setData(const char *fileName, int width, int height) {
    if (connect() < 0) {
        ALOGE("setData connect fail! !!!!!!!!!!!!!!!!!!!\n");
        return -1;
    }
    if (fileName == NULL) {
        return setRGBAData(width, height);
    }

    int size = width * height * 3/2;  
    unsigned char *data = new unsigned char[size];  
    FILE *fp = fopen(fileName,"rb");
    ALOGI("fopen %s\n", fileName);
    if(fp == NULL){  
        ALOGE("read %s fail !!!!!!!!!!!!!!!!!!!\n",fileName);
        delete []data;
        return -2;  
    }

    int nY4M = 0;
    if (strstr(fileName, ".y4m") != NULL) {
        ALOGI("%s is *.y4m\n", fileName);
        nY4M = 1;
        char buffer[6] = "FRAME";
        buffer[5] = '\0';
        char temp[10] = "\0" ;
        
        while(memcmp(buffer, temp, 5) != 0) { 
            if (fscanf(fp, "%s ", temp) <= 0)
               break;
            
            ALOGI("source %s dest %s\n", buffer, temp);   
        }
    }
    
    int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000, curTime, diff;
    for (int i = 0;; ++i) {
		if (getYUV12Data(fp, data, size, nY4M) != 0) {//get yuv data from file;
		    ALOGE("[%s][%d] count %d file read over\n",__FILE__,__LINE__, i);
		    break;
		}
        //render(data, size, width, height);
        setdata(inputProducer, HAL_DATASPACE_UNKNOWN, data, size, width, height);
        curTime = systemTime(CLOCK_MONOTONIC) / 1000;
        diff = curTime - nowTime;
        ALOGI("timestamp %lX %lX render\n", curTime / 1000, diff / 1000);
        if (diff < 16000)
           usleep(16000 - diff);
        nowTime = curTime;
    }
    
    fclose(fp);
    delete[]  data;

    return 0;
}

int SurfaceSource::onClose() {

    return 0;
}

int SurfaceSource::addOutput(sp<IGraphicBufferProducer> outputProducer) {
    IGraphicBufferProducer::QueueBufferOutput qbo;
    status_t err = outputProducer->connect(new DummyProducerListener, NATIVE_WINDOW_API_CPU, false,
            &qbo);
    if (err != NO_ERROR) {
        ALOGE("addOutput: failed to connect (%d)", err);
        return err;
    }

    mOutputs.push_back(outputProducer);
    return 0;
}

int SurfaceSource::StreamSplit(unsigned char *buffer, int sz) {
    // mFrameListener->waitForFrame();
    BufferItem item;
    inputConsumer->acquireBuffer(&item, 0);
    void* dataOut;
    item.mGraphicBuffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, &dataOut);
    memcpy(buffer, dataOut, sz);

    uint32_t stride = item.mGraphicBuffer->getStride();
    uint32_t nWidth = item.mGraphicBuffer->getWidth();
    uint32_t nHeight = item.mGraphicBuffer->getHeight();
    PixelFormat format = item.mGraphicBuffer->getPixelFormat();
    uint32_t bpp = android::bytesPerPixel(format);
    android_dataspace dataspace = item.mDataSpace;

    item.mGraphicBuffer->unlock();
    //size_t sz = stride * nHeight * bpp;
    //fprintf(stderr, " %d %d %d %d %d %zu\n", stride, nWidth, nHeight, bpp, format, sz);
    ALOGI("consumer_thread GraphicBuffer->lock %d %d %d %d %d %d\n", stride, nWidth, nHeight, bpp, format,sz);
    inputConsumer->detachBuffer(item.mSlot);
    inputConsumer->releaseBuffer(item.mSlot, item.mFrameNumber, EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, item.mFence);
    // Attach and queue the buffer to each of the outputs
    std::vector<sp<IGraphicBufferProducer> >::iterator output = mOutputs.begin();
    for (; output != mOutputs.end(); ++output) {
        setdata(*output, dataspace, buffer, sz, width, height);
    }
    return 0;
}

int  SurfaceSource::setBuffer(const void *data, size_t size, int width, int height, int64_t timestamp, int yuv) {
    size_t bufferIndex = -1;
    {
        Mutex::Autolock lock(mMutex);
        if (mInputBuffers.empty())
            return -1;
        bufferIndex = mInputBuffers.front();
        mInputBuffers.pop_front();
    }

    int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
    ALOGI("dequeueInputBuffer after %zu  %zu\n", nowTime, timestamp/1000000);
    sp<ABuffer> buffer;
    mEncoder->getInputBuffer(bufferIndex, &buffer);
    size_t offset = buffer->offset();
    ALOGI("buffer size %zu capacity %zu offset %zu  %zu\n", buffer->size(), buffer->capacity(), offset, size);
    if (yuv == 1) {
        memcpy(buffer->data(), data, size);
    } else {
        RGBA_To_YUV420SP((const unsigned char *)data, width, height, buffer->data());
    }
    buffer->setRange(offset, size);


    status_t err;
    err = mEncoder->queueInputBuffer(bufferIndex, offset, size, timestamp/1000, 0);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to queueInputBuffer (err=%d)\n", err);
    }
    nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
    ALOGI("queueInputBuffer size %zu pts %zu %zu\n", size, timestamp/1000000, nowTime);
    return 0;
}

int SurfaceSource::Splitter() {
    uint32_t       size ;
    int            yuv = 1;
    mFrameAvailableListener->waitForFrame();
    CpuConsumer::LockedBuffer buf;
    struct BufferInfo bufferInfo;
    bufferInfo.size = 4 * width * height ;
    mConsumer->lockNextBuffer(&buf);
    bufferInfo.stride = buf.stride;
    bufferInfo.width = buf.width;
    bufferInfo.height = buf.height;
    bufferInfo.format = buf.format;
    bufferInfo.timestamp = buf.timestamp;
    uint32_t bpp = android::bytesPerPixel(bufferInfo.format);        
    bufferInfo.size = bufferInfo.stride * bpp * bufferInfo.height;
    if (bufferInfo.format == HAL_PIXEL_FORMAT_YV12) {//HAL_PIXEL_FORMAT_YCbCr_420_888 HAL_PIXEL_FORMAT_YV12
        //bufferInfo.size = bufferInfo.stride * bufferInfo.height * 3/2;
        size = bufferInfo.size = bufferInfo.stride * bufferInfo.height * 3/2;
    } else {
        size = width * height * 3/2;
        yuv = 0;
    }

    bufferInfo.dataSpace = buf.dataSpace;
    bufferInfo.data = buf.data;
    setBuffer(buf.data, size, buf.width, buf.height, buf.timestamp, yuv);
    ALOGI("Splitter GraphicBuffer->lock %d %d %d %d %d %d\n", bufferInfo.stride, bufferInfo.width, bufferInfo.height, bpp, bufferInfo.format,bufferInfo.size);
    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
    ALOGI("SurfaceSource::Splitter %lld  %lld %lld", nowTime, bufferInfo.timestamp, (nowTime - bufferInfo.timestamp)/1000000);
    std::vector<sp<IGraphicBufferProducer> >::iterator output = mOutputs.begin();
    for (; output != mOutputs.end(); ++output) {
        setdata(*output, &bufferInfo);
    }
    mConsumer->unlockBuffer(buf);
    return 0;
}

int SurfaceSource::consumer_thread() {
    int64_t nowTime = systemTime(CLOCK_MONOTONIC) , curTime;
    while(true) {
        Splitter();
        curTime = systemTime(CLOCK_MONOTONIC);
        ALOGI("SurfaceSource::consumer_thread %lld  %lld  %lld", nowTime, curTime, (curTime - nowTime)/1000000);
        nowTime = curTime;
    }

    return 0;
}


int SurfaceSource::encode_input_thread() {
    int bitRate = 4000000;
    float displayFps = 60;
    const char* kMimeTypeAvc = MEDIA_MIMETYPE_VIDEO_AVC;
    sp<AMessage> format = new AMessage;
    format->setString("mime", kMimeTypeAvc);
    format->setInt32("width", width);
    format->setInt32("height", height);
    format->setInt32("color-format", OMX_COLOR_FormatYUV420SemiPlanar);// 
    format->setInt32("bitrate", bitRate);
    format->setFloat("frame-rate", displayFps);
    format->setInt32("i-frame-interval", 10);

    sp<ALooper> looper = new ALooper;
    looper->setName("apprecord_looper");
    looper->start();
    ALOGI("Create codec");
    sp<MediaCodec> encoder = MediaCodec::CreateByType(looper, kMimeTypeAvc, true);
    if (encoder == NULL) {
        ALOGE("ERROR: unable to create %s codec instance\n", kMimeTypeAvc);
        return -1;
    }
    status_t err;
    err = encoder->configure(format, NULL, NULL,
            MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to configure %s codec at %dx%d (err=%d)\n",
                kMimeTypeAvc, width, height, err);
        encoder->release();
        return -2;
    }

    ALOGI("Start codec");
    err = encoder->start();
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to start codec (err=%d)\n", err);
        encoder->release();
        return -3;
    }

    mEncoder = encoder;

    sp<AMessage> formatIn  = new AMessage;
    err = mEncoder->getInputFormat(&formatIn);
    ALOGI("InputFormat %d %s\n", err, formatIn->debugString().c_str());

    int kTimeout = 250000;   // be responsive on signal
    std::thread* thread = new std::thread(&SurfaceSource::encode_output_thread, this);
    mThreads.push_back(thread);
    int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000;

    size_t bufferIndex = -1;
    while(true) {
        nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
        ALOGI("dequeueInputBuffer %zu\n", nowTime);
        err = encoder->dequeueInputBuffer(&bufferIndex, kTimeout);
        if (err != NO_ERROR) {
            ALOGE("ERROR: unable to dequeueInputBuffer (err=%d)\n", err);
            continue;
        }
         
        {
            Mutex::Autolock lock(mMutex);
            mInputBuffers.push_back(bufferIndex);
        }

    }

    return 0;
}

int SurfaceSource::encode_output_thread() {
    FILE *fp = fopen("/sdcard/1.h264", "wb");

    if (fp == NULL) {
        mEncoder->release();
        ALOGE("fopen %s fp == NULL\n", "/sdcard/1.h264");
        return -1;
    }

    status_t err;
    sp<AMessage> formatOut = new AMessage;
    err = mEncoder->getOutputFormat(&formatOut);
    ALOGI("OutputFormat %d %s\n", err, formatOut->debugString().c_str());

    int kTimeout = 250000;   // be responsive on signal
    double fps = 0;
    int64_t useTime = systemTime(CLOCK_MONOTONIC) / 1000;
    while(true) {

        size_t bufIndex = -1, offset, size;
        int64_t ptsUsec, nowTime;
        uint32_t flags;

        while(true) {
             err = mEncoder->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
                &flags, kTimeout);
             ALOGI("dequeueOutputBuffer %zu returned %d\n", bufIndex, err);
             if (err == NO_ERROR) {
                sp<ABuffer> outBuffer;
                mEncoder->getOutputBuffer(bufIndex, &outBuffer);
                nowTime = systemTime(CLOCK_MONOTONIC) / 1000;
                ALOGI("Got data in buffer %zu, size=%zu, pts=%zu time=%zu delay=%zu\n",
                        bufIndex, size, ptsUsec, nowTime, (nowTime-ptsUsec)/1000);
                fps += 1;
                if (nowTime > useTime + 1000000) {
                    ALOGI("fps %f\n", fps * 1000000/(nowTime - useTime));
                    fps = 0;
                    useTime = nowTime; 
                }
                // If the virtual display isn't providing us with timestamps,
                // use the current time.  This isn't great -- we could get
                // decoded data in clusters -- but we're not expecting
                // to hit this anyway.
                if (ptsUsec == 0) {
                    ptsUsec = systemTime(SYSTEM_TIME_MONOTONIC) / 1000;
                }
                fwrite(outBuffer->data(), 1, size, fp);
                // Flush the data immediately in case we're streaming.
                // We don't want to do this if all we've written is
                // the SPS/PPS data because mplayer gets confused.
                if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) == 0) {
                    fflush(fp);
                } 
                err = mEncoder->releaseOutputBuffer(bufIndex);
                if (err != NO_ERROR) {
                    ALOGE("Unable to release output buffer (err=%d)\n",
                            err);
                    return err;
                }
                break;
             }
             
             if (err == INVALID_OPERATION) {
                ALOGE("dequeueOutputBuffer returned INVALID_OPERATION\n");
                return -2;
             }
             continue;
        }
    }

    fclose(fp);
    return 0;
}

int SurfaceSource::start() {
    std::thread* thread = new std::thread(&SurfaceSource::encode_input_thread, this);
    mThreads.push_back(thread);
    thread = new std::thread(&SurfaceSource::encode_output_thread, this);
    mThreads.push_back(thread);
    return 0;
}


int SurfaceSource::RGBA_To_YUV420SP(const unsigned char *rgba, int width, int height, unsigned char* yuv420sp) {
    int i = 0, j = 0;
    int r = 0, g = 0, b = 0;
    int y = 0, u = 0, v = 0;
    unsigned char *yuv = yuv420sp, *uv = yuv420sp + width * height;
    for (j = 0 ; j < height ; ++j) {
       for (i = 0 ; i < width; ++i, rgba += 4 , ++yuv) {
           r = rgba[0];
           g = rgba[1];
           b = rgba[2];
           y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
           u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
           v = ((112 * r - 94 * g + 18 * b + 128) >> 8) + 128;
           y = (y < 0) ? 0 : y;
           y = (y > 255) ? 255 : y;
           yuv[0] = (unsigned char)y;
           if (((i % 2) != 0) || ((j % 2) != 0))
              continue;
           u = (u < 0) ? 0 : u;
           u = (u > 255) ? 255 : u;
           v = (v < 0) ? 0 : v;
           v = (v > 255) ? 255 : v;
           uv[0] = (unsigned char)u;
           uv[1] = (unsigned char)v;
           uv += 2;
       }
    }
    return 0;
}