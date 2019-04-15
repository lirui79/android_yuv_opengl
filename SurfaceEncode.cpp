
#include <utils/Log.h>
#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/PersistentSurface.h>
#include <media/stagefright/MediaMuxer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/ICrypto.h>
#include "SurfaceEncode.h"

SurfaceEncode::SurfaceEncode() {
}

SurfaceEncode::~SurfaceEncode() {
}

int SurfaceEncode::onCreate(int w, int h, int Yuv) {
    width  = w ;
    height = h ;
    yuv = Yuv;
    if (yuv == 1)
        return onCreateYuv(w, h);
    return onCreateSurface(w, h);
}

int SurfaceEncode::onCreateYuv(int w, int h) {
    BufferQueue::createBufferQueue(&inputProducer, &inputConsumer);
    mFrameListener = new FrameListener();
    sp<IConsumerListener> proxy = new BufferQueue::ProxyConsumerListener(mFrameListener);
    inputConsumer->consumerConnect(proxy, false);
    inputConsumer->setDefaultBufferFormat(HAL_PIXEL_FORMAT_YV12);//HAL_PIXEL_FORMAT_RGBA_8888 ?HAL_PIXEL_FORMAT_YV12
    inputConsumer->setDefaultBufferSize(width, height);
    return 0;
}


int SurfaceEncode::onCreateSurface(int w, int h) {
    int bitRate = 4000000;
    float displayFps = 60;
    const char* kMimeTypeAvc = MEDIA_MIMETYPE_VIDEO_AVC;// "video/avc";MEDIA_MIMETYPE_VIDEO_VP9
    sp<AMessage> format = new AMessage;
    format->setInt32("width", width);
    format->setInt32("height", height);
    format->setString("mime", kMimeTypeAvc);
//   format->setInt32("color-format", OMX_COLOR_FormatYUV420Flexible);
    format->setInt32("color-format", OMX_COLOR_FormatAndroidOpaque);
    format->setInt32("bitrate", bitRate);
    format->setFloat("frame-rate", displayFps);
    format->setInt32("i-frame-interval", 10);

    sp<ALooper> looper = new ALooper;
    looper->setName("screenrecord_looper");
    looper->start();
    //fprintf(stderr, "Creating codec\n");
    ALOGI("Create codec");
    sp<MediaCodec> codec = MediaCodec::CreateByType(looper, kMimeTypeAvc, true);
    if (codec == NULL) {
     //   fprintf(stderr, "ERROR: unable to create %s codec instance\n", kMimeTypeAvc);
        ALOGE("ERROR: unable to create %s codec instance\n", kMimeTypeAvc);
        return -1;
    }
    status_t err;
    err = codec->configure(format, NULL, NULL,
            MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != NO_ERROR) {
     //   fprintf(stderr, "ERROR: unable to configure %s codec at %dx%d (err=%d)\n",
        ALOGE("ERROR: unable to configure %s codec at %dx%d (err=%d)\n",
                kMimeTypeAvc, width, height, err);
        codec->release();
        return -2;
    }

    //fprintf(stderr, "Creating encoder input surface\n");
    ALOGI("createInputSurface");
    sp<IGraphicBufferProducer> bufferProducer;
    err = codec->createInputSurface(&bufferProducer);
    if (err != NO_ERROR) {
        //fprintf(stderr,
        ALOGE("ERROR: unable to create encoder input surface (err=%d)\n", err);
        codec->release();
        return -3;
    }

   // fprintf(stderr, "Starting codec\n");
    ALOGI("Starting codec");
    err = codec->start();
    if (err != NO_ERROR) {
        //fprintf(stderr, "ERROR: unable to start codec (err=%d)\n", err);
        ALOGE("ERROR: unable to start codec (err=%d)\n", err);
        codec->release();
        return -4;
    }
    encoder = codec;
    inputProducer = bufferProducer;
    //fprintf(stderr, "Codec prepared\n");
    ALOGI("MediaCodec prepared");
    sp<AMessage> formatIn  = new AMessage;
    err = encoder->getInputFormat(&formatIn);
  //  fprintf(stderr, "in %d %s\n", err, formatIn->debugString().c_str());
    ALOGI("InputFormat %d %s\n", err, formatIn->debugString().c_str());
    return 0;
}

int SurfaceEncode::encode_input_thread() {
    int bitRate = 4000000;
    float displayFps = 60;
    const char* kMimeTypeAvc = MEDIA_MIMETYPE_VIDEO_AVC;//MEDIA_MIMETYPE_VIDEO_AVC "video/avc";MEDIA_MIMETYPE_VIDEO_VP9
//    const char* kMimeTypeAvc = "video/avc";
    sp<AMessage> format = new AMessage;
    format->setString("mime", kMimeTypeAvc);
    format->setInt32("width", width);
    format->setInt32("height", height);
    format->setInt32("color-format", OMX_COLOR_FormatYUV420SemiPlanar);
//    format->setInt32("color-format", OMX_COLOR_FormatAndroidOpaque);//OMX_COLOR_FormatAndroidOpaque
    format->setInt32("bitrate", bitRate);
    format->setFloat("frame-rate", displayFps);
    format->setInt32("i-frame-interval", 10);

    sp<ALooper> looper = new ALooper;
    looper->setName("apprecord_looper");
    looper->start();
 //   fprintf(stderr, "Creating codec\n");
    ALOGI("Create codec");
    sp<MediaCodec> codec = MediaCodec::CreateByType(looper, kMimeTypeAvc, true);
    if (codec == NULL) {
    //    fprintf(stderr, "ERROR: unable to create %s codec instance\n", kMimeTypeAvc);
        ALOGE("ERROR: unable to create %s codec instance\n", kMimeTypeAvc);
        return -1;
    }
    status_t err;
    err = codec->configure(format, NULL, NULL,
            MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != NO_ERROR) {
        //fprintf(stderr, "ERROR: unable to configure %s codec at %dx%d (err=%d)\n",
        ALOGE("ERROR: unable to configure %s codec at %dx%d (err=%d)\n",
                kMimeTypeAvc, width, height, err);
        codec->release();
        return -2;
    }

   // fprintf(stderr, "Starting codec\n");
    ALOGI("Start codec");
    err = codec->start();
    if (err != NO_ERROR) {
      //  fprintf(stderr, "ERROR: unable to start codec (err=%d)\n", err);
        ALOGE("ERROR: unable to start codec (err=%d)\n", err);
        codec->release();
        return -3;
    }

    encoder = codec;

    sp<AMessage> formatIn  = new AMessage;
    err = encoder->getInputFormat(&formatIn);
    //fprintf(stderr, "in %d %s\n", err, formatIn->debugString().c_str());
    ALOGI("InputFormat %d %s\n", err, formatIn->debugString().c_str());

    int kTimeout = 250000;   // be responsive on signal
    std::thread* thread = new std::thread(&SurfaceEncode::encode_output_thread, this);
    mThreads.push_back(thread);
    int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000;
    BufferItem  bufferItem;
    size_t sz = width * height * 3 / 2;
    unsigned char *dataIn = new unsigned char[sz];
    while(1) {
        //fprintf(stderr, "encode\n");
        mFrameListener->waitForFrame();

        nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
        //fprintf(stderr, "acquireBuffer %zu\n", nowTime);
        ALOGI("acquireBuffer %zu\n", nowTime);
        err = inputConsumer->acquireBuffer(&bufferItem, 0);
        if (err != NO_ERROR) {
            //fprintf(stderr, "ERROR: unable to acquireBuffer (err=%d)\n", err);
            ALOGE("ERROR: unable to acquireBuffer (err=%d)\n", err);
            continue;
        }
        nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
        //fprintf(stderr, "acquireBuffer after %zu\n", nowTime);
        ALOGI("acquireBuffer after %zu\n", nowTime);

        void* dataOut;
        bufferItem.mGraphicBuffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, &dataOut);
        uint32_t stride = bufferItem.mGraphicBuffer->getStride();
        uint32_t nWidth = bufferItem.mGraphicBuffer->getWidth();
        uint32_t nHeight = bufferItem.mGraphicBuffer->getHeight();
        PixelFormat format = bufferItem.mGraphicBuffer->getPixelFormat();
        uint32_t bpp = android::bytesPerPixel(format);
        int64_t timestamp = bufferItem.mTimestamp / 1000;

        memcpy(dataIn, dataOut, sz);
        bufferItem.mGraphicBuffer->unlock();

        inputConsumer->detachBuffer(bufferItem.mSlot);           
        nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
        ALOGI("releaseBuffer %zu\n", nowTime);
        err = inputConsumer->releaseBuffer(bufferItem.mSlot, bufferItem.mFrameNumber,
                EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, bufferItem.mFence);

        //size_t sz = stride * nHeight * bpp;
        //fprintf(stderr, " %d %d %d %d %d %zu\n", stride, nWidth, nHeight, bpp, format, sz);
        ALOGI("GraphicBuffer->lock %d %d %d %d %d %zu %lld\n", stride, nWidth, nHeight, bpp, format, sz, timestamp);
        size_t bufferIndex = -1;
        while(true) {

            nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
            //fprintf(stderr, "dequeueInputBuffer %zu\n", nowTime);
            ALOGI("dequeueInputBuffer %zu\n", nowTime);
            err = encoder->dequeueInputBuffer(&bufferIndex, kTimeout);
            if (err != NO_ERROR) {
                //fprintf(stderr, "ERROR: unable to dequeueInputBuffer (err=%d)\n", err);
                ALOGE("ERROR: unable to dequeueInputBuffer (err=%d)\n", err);
                continue;
            }
            nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
            //fprintf(stderr, "dequeueInputBuffer after %zu\n", nowTime);
            ALOGI("dequeueInputBuffer after %zu\n", nowTime);
            sp<ABuffer> buffer;
            encoder->getInputBuffer(bufferIndex, &buffer);
            nowTime = systemTime(CLOCK_MONOTONIC) / 1000000;
            //fprintf(stderr, "getInputBuffer %zu\n", nowTime);
            ALOGI("getInputBuffer %zu\n", nowTime);
            size_t offset = buffer->offset();
            // fprintf(stderr,"buffer size %zu capacity %zu offset %zu  %zu\n", buffer->size(), buffer->capacity(), offset, sz);
            ALOGI("buffer size %zu capacity %zu offset %zu  %zu\n", buffer->size(), buffer->capacity(), offset, sz);
            memcpy(buffer->data(), dataIn, sz);
            buffer->setRange(offset, sz);

            nowTime = systemTime(CLOCK_MONOTONIC) / 1000;
            //fprintf(stderr, "queueInputBuffer sz %zu pts %zu %zu\n", sz, nowTime, nowTime/1000);
            ALOGI("queueInputBuffer sz %zu pts %zu %zu\n", sz, nowTime, nowTime/1000);
            err = encoder->queueInputBuffer(bufferIndex, offset, sz, timestamp, 0);
            if (err != NO_ERROR) {
                //fprintf(stderr, "ERROR: unable to queueInputBuffer (err=%d)\n", err);
                ALOGE("ERROR: unable to queueInputBuffer (err=%d)\n", err);
            }
            break;
        }
    }

    return 0;
}

int SurfaceEncode::encode_output_thread() {
    FILE *fp = fopen("/sdcard/1.h264", "wb");
//    FILE *fp = fopen("/sdcard/1.h264", "wb");

    if (fp == NULL) {
        encoder->release();
        //fprintf(stderr, "fopen fp == NULL\n");
        ALOGE("fopen %s fp == NULL\n", "/sdcard/1.h264");
        return -1;
    }

    status_t err;
    sp<AMessage> formatOut = new AMessage;
    err = encoder->getOutputFormat(&formatOut);
    //fprintf(stderr, "out %d %s\n", err, formatOut->debugString().c_str());
    ALOGI("OutputFormat %d %s\n", err, formatOut->debugString().c_str());

    int kTimeout = 250000;   // be responsive on signal
    double fps = 0;
    int64_t useTime = systemTime(CLOCK_MONOTONIC) / 1000;
    while(true) {

        size_t bufIndex = -1, offset, size;
        int64_t ptsUsec, nowTime;
        uint32_t flags;

        while(true) {
             err = encoder->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
                &flags, kTimeout);
             ALOGI("dequeueOutputBuffer %zu returned %d\n", bufIndex, err);
             if (err == NO_ERROR) {
                sp<ABuffer> outBuffer;
                encoder->getOutputBuffer(bufIndex, &outBuffer);
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
                err = encoder->releaseOutputBuffer(bufIndex);
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


sp<IGraphicBufferProducer> SurfaceEncode::getIGraphicBufferProducer() const {
    return inputProducer;
}

int SurfaceEncode::start() {
    if (yuv == 1) {
        std::thread* thread = new std::thread(&SurfaceEncode::encode_input_thread, this);
        mThreads.push_back(thread);
        return 0;
    }

    std::thread* thread = new std::thread(&SurfaceEncode::encode_output_thread, this);
    mThreads.push_back(thread);
    return 0;
}