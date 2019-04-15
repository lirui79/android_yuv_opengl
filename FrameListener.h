
#ifndef FRAME_LISTENER_H
#define FRAME_LISTENER_H

#include <gui/IConsumerListener.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

using namespace android;

class FrameListener : public android::ConsumerListener {
public:
    FrameListener();
    virtual ~FrameListener();
 
    void onFrameAvailable(const BufferItem& /*item*/) ; /* Asynchronous */
 
    void onBuffersReleased(); /* Asynchronous */
 
    void onSidebandStreamChanged(); /* Asynchronous */
        
    void waitForFrame();
    
private:
    int mPendingFrames;
    Mutex mMutex;
    Condition mCondition;
};

#endif //FRAME_LISTENER_H