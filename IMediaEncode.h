#ifndef __IMEDIA_ENCODE_H_
#define __IMEDIA_ENCODE_H_

#include <stdint.h>
#include <sys/cdefs.h>
#include <jni.h>
#include <android/native_activity.h>
#include <EGL/egl.h>

/**
 * @addtogroup sdk interface
 * @{
 */

__BEGIN_DECLS

typedef struct IMediaEncode {

    void* (*onCreate)(struct IMediaEncode *encode, const char*name, int width, int height, int type);

    int (*onEncode)(struct IMediaEncode *encode);

    int (*onSetData)(struct IMediaEncode *encode, const char*name, int width, int height);
    
} IMediaEncode;

typedef struct IMediaEncode* (*getIMediaEncodeType)() ;
__attribute__((visibility("default"))) struct IMediaEncode* getIMediaEncode();



__END_DECLS

/**
* @}
*/


#endif //__IMEDIA_ENCODE_H_