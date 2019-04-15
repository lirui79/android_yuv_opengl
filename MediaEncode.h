#ifndef __MEDIA_ENCODE_H_
#define __MEDIA_ENCODE_H_

#include "IMediaEncode.h"
#include "SurfaceSource.h"
#include "SurfaceView.h"
#include "SurfaceEncode.h"


class MediaEncode : public IMediaEncode {
private:
    MediaEncode(const MediaEncode& module);

    MediaEncode& operator=(const MediaEncode& module);

protected:
    MediaEncode();

public:
    virtual ~MediaEncode();
    /**
     * @brief IAppManagerEx 单实例函数
     *
     * @return 返回说明
     *        -<em>NULL</em>   错误
     *        -<em>非NULL</em> 单实例智能指针
     *
     * @example  IAppManagerEx::singleton();
     */
    static MediaEncode* singleton();

    void* onCreateANativeWindow(const char*name, int width, int height, int type);

    int   startEncode();

    int   onSetData(const char*name, int width, int height);

protected:
    static void* MediaEncode_static_onCreate(struct IMediaEncode *iencode, const char* name, int width, int height, int type) {
        MediaEncode *encode = (MediaEncode*) iencode;
        return encode->onCreateANativeWindow(name, width, height, type);
    }

    static int MediaEncode_static_onEncode(struct IMediaEncode *iencode) {
        MediaEncode *encode = (MediaEncode*) iencode;
        return encode->startEncode();
    }

    static int MediaEncode_static_onSetData(struct IMediaEncode *iencode, const char* name, int width, int height) {
        MediaEncode *encode = (MediaEncode*) iencode;
        return encode->onSetData(name, width, height);
    }

private:
    SurfaceSource   mSource;
    SurfaceView     mView;
//    SurfaceEncode   mEncode;
};


#endif // __MEDIA_ENCODE_H_