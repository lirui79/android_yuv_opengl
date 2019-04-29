

#include "SurfaceView.h"
#include <utils/Log.h>

SurfaceView::SurfaceView() {
}

SurfaceView::~SurfaceView() {
}

int SurfaceView::createSurface(const char *name, int width, int height, int x, int y) {
 // create a client to surfaceflinger
    client = new SurfaceComposerClient();
    int layerStack = 0; 
#ifdef PLATFORM_VERSION_7
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdMain));
    layerStack = 0; 
#endif

#ifdef PLATFORM_VERSION_8
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdHdmi));
    layerStack = 10; 
#endif

    DisplayInfo dinfo;
    //获取屏幕的宽高等信息
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    ALOGI("w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f,orientation=%d\n",
        dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density,dinfo.orientation);
    if (status)
        return -1;

    Rect crop(0, 0, width, height);
	//float x = crops[0], y = crops[1];
    if (dinfo.orientation) {
		uint32_t temp = width ;
		width = height;
		height = temp;
        crop = Rect(0, 0, width, height);
        temp = x;
        x = y;
        y = temp;
		//x = crops[1];
		//y = crops[0];
	}

    int zOrder = 300000;
   // client->setDisplayProjection(dtoken, DISPLAY_ORIENTATION_90, destRect, destRect);

    surfaceControl = client->createSurface(String8(name),
            width, height, PIXEL_FORMAT_RGBA_8888, 0);
	SurfaceComposerClient::setDisplayLayerStack(dtoken, layerStack); 
    SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayerStack(layerStack); 
    surfaceControl->setLayer(zOrder);//设定Z坐标
    surfaceControl->setCrop(crop);
    surfaceControl->setPosition(x, y);//以左上角为(0,0)设定显示位置
    surfaceControl->show();//感觉没有这步,图片也能显示
    SurfaceComposerClient::closeGlobalTransaction();
    surface = surfaceControl->getSurface();
    return 0;
}

sp<IGraphicBufferProducer> SurfaceView::getIGraphicBufferProducer() const {
    return surface->getIGraphicBufferProducer();
}
