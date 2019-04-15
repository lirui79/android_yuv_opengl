 
 
 
#include "FrameListener.h"
#include <stdio.h>
#include <gui/BufferItem.h>
#include <ui/GraphicBuffer.h>
#include <utils/Log.h>
 
 
FrameListener::FrameListener() : mPendingFrames(0) {
}
 
FrameListener::~FrameListener() {
}
 
void FrameListener::onFrameAvailable(const BufferItem& /*item*/) {
 //   fprintf(stderr, "onFrameAvailable\n");
    ALOGI("FrameListener::onFrameAvailable");
    Mutex::Autolock lock(mMutex);
    ++mPendingFrames;
    mCondition.signal();
}
 
void FrameListener::onBuffersReleased() {
 //   fprintf(stderr, "onBuffersReleased\n"); 
    ALOGI("FrameListener::onBuffersReleased");         
 
}
 
 
void FrameListener::onSidebandStreamChanged() {
 //   fprintf(stderr, "onSidebandStreamChanged\n");  
    ALOGI("FrameListener::onSidebandStreamChanged");   
 
}

void FrameListener::waitForFrame() {
  //  fprintf(stderr, "waitForFrame\n"); 
    ALOGI("FrameListener::waitForFrame");   
    Mutex::Autolock lock(mMutex);
    if (mPendingFrames <= 0) {
        mCondition.wait(mMutex);
    }
    --mPendingFrames;
}