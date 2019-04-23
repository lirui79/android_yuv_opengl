#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#include <utils/Log.h>
#include "IMediaEncode.h"
#include <binder/IPCThreadState.h>  
#include <binder/ProcessState.h>

#include <ui/GraphicBuffer.h>


#include <dlfcn.h>

using namespace android;

GLuint programID = -1;// program id
GLuint vertexShader = -1; // vertex shader id
GLuint fragmentShader = -1; // fragment shader id

GLuint vertexID  = -1;// vertex  id
GLuint textureID = -1;// texture id

GLint  vertexLoc = -1;// gpu vertex handle
GLint  textureLoc = -1;//gpu texture handle
GLint  yuvtextLoc = -1;//gpu yuv texture handle
GLint drawOrder[] = { 0, 1, 2, 0, 2, 3 }; 
// vertex array
GLfloat vextexData[] = {
       -1.0f,  -1.0f,
       1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f,  1.0f,
};
//顶点颜色数组  
float colorData[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
};

const char* vertexShaderCode =
            "attribute vec4 aPosition;\n"
            "attribute vec2 aTextureCoord;\n"
            "varying vec2   vTextureCoord;\n"
            "void main()\n"
            "{\n"
                "gl_Position = aPosition;\n"
                "vTextureCoord = aTextureCoord;\n"
            "}";

const char* fragmentShaderCode =
            "#extension GL_OES_EGL_image_external : require\n"
            "precision mediump float;\n"
            "varying vec2 vTextureCoord;\n"
            "uniform samplerExternalOES yuvTextureID;\n"
            "void main() {\n"
            "  gl_FragColor = texture2D(yuvTextureID, vTextureCoord);\n"
            "}";


int useShader(const char *shader, GLenum type, GLuint *vShader) {
//创建着色器对象：顶点着色器  
	  vShader[0] = glCreateShader(type);
//将字符数组绑定到对应的着色器对象上
	  glShaderSource(vShader[0], 1, &shader, NULL);
//编译着色器对象  
    glCompileShader(vShader[0]);
//检查编译是否成功  
    GLint compileResult;
    glGetShaderiv(vShader[0], GL_COMPILE_STATUS, &compileResult);
    if (GL_FALSE == compileResult) {
        printf("useShader %s %d %d fail\n", shader, type, vShader[0]);
        glDeleteShader(vShader[0]);
        return -1;
    }
    return 0;
}

int useProgram() {
    if (useShader(vertexShaderCode, GL_VERTEX_SHADER, &vertexShader) < 0) {
      return -1;// vertex shader id
    }

    if (useShader(fragmentShaderCode, GL_FRAGMENT_SHADER, &fragmentShader) < 0) {
      return -1;// fragment shader id
    }

    programID = glCreateProgram() ;// program id

    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);

    glLinkProgram(programID);

    vertexLoc = glGetAttribLocation(programID, "aPosition");
    textureLoc = glGetAttribLocation(programID, "aTextureCoord");
    yuvtextLoc = glGetUniformLocation(programID, "yuvTextureID");

    return 0;
}

GLuint useTextureID() {
    GLuint texture = -1;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

int draw() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(programID);

    glVertexAttribPointer(vertexLoc, 2, GL_FLOAT, GL_FALSE, 0, vextexData);
    glEnableVertexAttribArray(vertexLoc);

    glVertexAttribPointer(textureLoc, 2, GL_FLOAT, GL_FALSE, 0, colorData);
    glEnableVertexAttribArray(textureLoc);

    glUniform1i(yuvtextLoc, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, drawOrder);

    glDisableVertexAttribArray(vertexLoc);
    glDisableVertexAttribArray(textureLoc);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    glUseProgram(0);
  return 0;
}



static void *GetFunction(const char *dllName, const char *functionName){
	const char *error;
	if ((error = dlerror()) != NULL) {
		ALOGI("Before GetNewClass [%s]-[%s] reports error %s", dllName , functionName , error);
	}
	void *lib_handle = dlopen(dllName, RTLD_LAZY);
	if (!lib_handle) {
		ALOGI("dlopen [%s]-[%s] reports error %s", dllName , functionName , dlerror());
		return NULL;
	}
	void* fn = dlsym(lib_handle, functionName);
	if (fn == NULL) {
	    if ((error = dlerror()) != NULL)  {
	    	ALOGI("dlsym [%s]-[%s] reports error %s", dllName , functionName , error);
	    } else {
	    	ALOGI("dlsym [%s]-[%s] reports error return NULL", dllName , functionName);
	    }
		return NULL;
	}

	if ((error = dlerror()) != NULL) {
		ALOGI("dlsym [%s]-[%s] reports error %s", dllName , functionName , error);
	}
	return fn;
}



int initDisplay(EGLDisplay *display, const EGLint* configAttribs, EGLConfig* config) {
    display[0] = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint majorVersion;
    EGLint minorVersion;
    eglInitialize(display[0], &majorVersion, &minorVersion);
    EGLint numConfigs = 0;
    eglChooseConfig(display[0], configAttribs, config, 1, &numConfigs);
    return 0;
}


int getYUV12Data(FILE *fp, unsigned char * pYUVData,int size, int nY4M) {
    int sz = fread(pYUVData,size,1,fp);
    if (nY4M != 0) {
         unsigned char temp[6] = "12345" ;
         fread(temp,6,1,fp);
    }
    if (sz > 0)
       return 0;
    return -1;
}



//testVB 352 288 /sdcard/akiyo_cif.yuv
int main(int argc, char** argv)  
{   
    if (argc < 3) {
   
      printf("please input w h and yuv file\n");
      
      return -1;
    }
    
    printf("input w %s h %s" , argv[1] , argv[2]);
    int width  = atoi(argv[1]);
    int height = atoi(argv[2]);
    const char *yuvfile = argv[3];
    int yuv = 0;
   /* 
    if (argc == 4) {
        yuvfile = argv[3] ;
        yuv = 1;
        printf(" yuv file %s\n" , argv[3]);
        getIMediaEncodeType getIMediaEncodeFn = (getIMediaEncodeType) GetFunction("libMediaEncode.so", "getIMediaEncode");
        IMediaEncode *mediaEncode = getIMediaEncodeFn();
        ANativeWindow* window = (ANativeWindow*) mediaEncode->onCreate(mediaEncode, "media surface", width, height, yuv);
      
        mediaEncode->onEncode(mediaEncode);

        mediaEncode->onSetData(mediaEncode, yuvfile, width, height);

    } else {*/
        const EGLint configAttribs[] = {
          EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
          EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
          EGL_RED_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_BLUE_SIZE, 8,
          EGL_ALPHA_SIZE, 8,
          EGL_NONE };

        const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE };

        EGLDisplay mEglDisplay = EGL_NO_DISPLAY;
        EGLSurface mEglSurface = EGL_NO_SURFACE;
        EGLContext mEglContext = EGL_NO_CONTEXT;
        EGLConfig  mGlConfig = NULL;

        if (initDisplay(&mEglDisplay, configAttribs, &mGlConfig) < 0) {
          printf("opengl initdisplay error!\n");
          return -2;
        }
        printf("\n");

        getIMediaEncodeType getIMediaEncodeFn = (getIMediaEncodeType) GetFunction("libMediaEncode.so", "getIMediaEncode");
        IMediaEncode *mediaEncode = getIMediaEncodeFn();
        ANativeWindow* window = (ANativeWindow*) mediaEncode->onCreate(mediaEncode, "media surface", width, height, yuv);

        mediaEncode->onEncode(mediaEncode);

        mEglSurface = eglCreateWindowSurface(mEglDisplay, mGlConfig, window , NULL);
        mEglContext = eglCreateContext(mEglDisplay, mGlConfig, EGL_NO_CONTEXT, contextAttribs);
        eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);


        const int yuvTexUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                GraphicBuffer::USAGE_SW_WRITE_RARELY;
        const int yuvTexFormat = HAL_PIXEL_FORMAT_YV12;
        GraphicBuffer*  yuvTexBuffer = new GraphicBuffer(width, height, yuvTexFormat, yuvTexUsage);
        EGLClientBuffer clientBuffer = (EGLClientBuffer)yuvTexBuffer->getNativeBuffer();
        EGLImageKHR img = eglCreateImageKHR(mEglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, 0);
        
        textureID = useTextureID();
        glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img);


        if (useProgram() < 0) {
          return -1;
        }
        glViewport(0, 0, width, height);


        FILE *fp = fopen(yuvfile,"rb");
        printf("fopen %s\n", yuvfile);
        if(fp == NULL){  
            printf("read %s fail !!!!!!!!!!!!!!!!!!!\n",yuvfile);
            return -2;  
        }

        int nY4M = 0;
        if (strstr(yuvfile, ".y4m") != NULL) {
            printf("%s is *.y4m\n", yuvfile);
            nY4M = 1;
            char buffer[6] = "FRAME";
            buffer[5] = '\0';
            char temp[10] = "\0" ;
            
            while(memcmp(buffer, temp, 5) != 0) { 
                if (fscanf(fp, "%s ", temp) <= 0)
                  break;
                
                printf("source %s dest %s\n", buffer, temp);   
            }
        }
        int size = width * height * 3/2;  
        int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000, curTime, diff;

        unsigned char* buf = NULL;
        status_t err; 
        for (int i = 0;; ++i) {
            err = yuvTexBuffer->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&buf));

            if (getYUV12Data(fp, buf, size, nY4M) != 0) {//get yuv data from file;
                printf("[%s][%d] count %d file read over\n",__FILE__,__LINE__, i);
                break;
            }
            err = yuvTexBuffer->unlock();

            draw();
            eglSwapBuffers(mEglDisplay, mEglSurface);
            curTime = systemTime(CLOCK_MONOTONIC) / 1000;
            diff = curTime - nowTime;
            printf("timestamp %lX %lX render\n", curTime / 1000, diff / 1000);
            if (diff < 16000)
              usleep(16000 - diff);
            nowTime = curTime;
        }
        
        fclose(fp);

/*

        int64_t nowTime = systemTime(CLOCK_MONOTONIC) , curTime, diff;
        for (int i = 0 ; i < 1000000; ++i) {
            switch (i % 3) {
              case 0:
                glClearColor(0.3, 0, 0, 0.2);
                break;
              case 1:
                glClearColor(0, 0.5, 0, 0.1);
                break;
              case 2:
                glClearColor(0, 0, 0.1, 0.5);
                break;
            }
            //

            glClear(GL_COLOR_BUFFER_BIT);
            //glColor4f(1.0, 1.0, 1.0, 1.0) ;
           // glBegin(GL_LINES);


           // glEnd();
            //nowTime = systemTime(CLOCK_MONOTONIC) ;
            eglSwapBuffers(mEglDisplay, mEglSurface);
            curTime = systemTime(CLOCK_MONOTONIC);
            diff = (curTime - nowTime)/1000;
            ALOGI("eglSwapBuffers %lld  %lld  %lld", nowTime, curTime, diff/1000);
          //  printf("timestamp %zu %zu render\n", curTime / 1000, diff / 1000);
            if (diff < 16000) {
              usleep(16000 - diff);
              nowTime = systemTime(CLOCK_MONOTONIC);
            }
            nowTime = curTime;
        }*/

  //  }

    IPCThreadState::self()->joinThreadPool();      
    IPCThreadState::self()->stopProcess();  
    return 0;
}