

#include "SurfaceView.h"
#include <utils/Log.h>

SurfaceView::SurfaceView() {
}

SurfaceView::~SurfaceView() {
}

int SurfaceView::createSurface(const char *name, int width, int height, int x, int y) {
 // create a client to surfaceflinger  
    client = new SurfaceComposerClient();  
    //DisplayoutBuffer display;  
    //client->getDisplayoutBuffer(client->getBuiltInDisplay(HWC_DISPLAY_PRIMARY), &display);  
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdMain));  
    DisplayInfo dinfo;  
    //获取屏幕的宽高等信息  
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);  
    ALOGI("w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f,orientation=%d\n",   
        dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density,dinfo.orientation);  
    if (status)  
        return -1;

    if (dinfo.w < width) {
        int temp = dinfo.h;
        dinfo.h = dinfo.w ;
        dinfo.w = temp;
    }
    Rect destRect(dinfo.w, dinfo.h);

   // client->setDisplayProjection(dtoken, DISPLAY_ORIENTATION_90, destRect, destRect);
    
    surfaceControl = client->createSurface(String8(name),  
            width, height, PIXEL_FORMAT_RGBA_8888, 0);
    surface = surfaceControl->getSurface();
    SurfaceComposerClient::openGlobalTransaction();  
    surfaceControl->setLayer(100000);//设定Z坐标  
    surfaceControl->setPosition(x, y);//以左上角为(0,0)设定显示位置  
    SurfaceComposerClient::closeGlobalTransaction();  
    surfaceControl->show();//感觉没有这步,图片也能显示
    return 0;
}

sp<IGraphicBufferProducer> SurfaceView::getIGraphicBufferProducer() const {
    return surface->getIGraphicBufferProducer();
}