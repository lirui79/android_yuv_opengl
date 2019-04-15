#ifndef SURFACE_VIEW_H
#define SURFACE_VIEW_H


  
#include <gui/Surface.h>  
#include <gui/SurfaceComposerClient.h>  
#include <gui/ISurfaceComposer.h>  
#include <ui/DisplayInfo.h>  

using namespace android;

class SurfaceView {
public:
    SurfaceView();
    virtual ~SurfaceView();

    int createSurface(const char *name, int width, int height, int x, int y);

    sp<IGraphicBufferProducer> getIGraphicBufferProducer() const;

private:
    sp<Surface> surface;
    sp<SurfaceComposerClient> client;
    sp<SurfaceControl> surfaceControl;
};

#endif //SURFACE_VIEW_H