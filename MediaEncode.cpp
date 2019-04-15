#include "MediaEncode.h"
#include <utils/Mutex.h>
#include <binder/IPCThreadState.h>  
#include <binder/ProcessState.h>  
#include <binder/IServiceManager.h> 

MediaEncode::MediaEncode() {
    IMediaEncode::onCreate = MediaEncode_static_onCreate;
    IMediaEncode::onEncode = MediaEncode_static_onEncode;
    IMediaEncode::onSetData = MediaEncode_static_onSetData;
}

MediaEncode::~MediaEncode() {
}
    /**
     * @brief IAppManagerEx 单实例函数
     *
     * @return 返回说明
     *        -<em>NULL</em>   错误
     *        -<em>非NULL</em> 单实例智能指针
     *
     * @example  IAppManagerEx::singleton();
     */
MediaEncode* MediaEncode::singleton() {
    static Mutex mMutex;
    static MediaEncode* singleton(NULL);
    if (singleton != NULL)
        return singleton;
    Mutex::Autolock lock(mMutex);
    if (singleton == NULL) 
        singleton = new MediaEncode();
    return singleton; 
}

void* MediaEncode::onCreateANativeWindow(const char*name, int width, int height, int type) {
    sp<ProcessState> proc(ProcessState::self());  
    ProcessState::self()->startThreadPool();

    mSource.onCreate(width, height, type);
    mView.createSurface(name, width, height, 0, 0);
    mSource.addOutput(mView.getIGraphicBufferProducer());
//    mEncode.onCreate(width, height, type);
//    mSource.addOutput(mEncode.getIGraphicBufferProducer());
    
    return mSource.getNativeWindow();
}

int   MediaEncode::startEncode() {
//    return mEncode.start();
    return mSource.start();
}

int   MediaEncode::onSetData(const char*name, int width, int height) {
   return  mSource.setData(name, width, height);
}



__BEGIN_DECLS

__attribute__((visibility("default"))) struct IMediaEncode* getIMediaEncode() {
    return (struct IMediaEncode*) (MediaEncode::singleton());
}

__END_DECLS