/*
 * Copyright (C) 2021 The Android Open Source Project
 */

#define LOG_TAG "NativeSFDemo"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <hardware/gralloc.h>
#include <ui/GraphicBuffer.h>
#include <utils/Log.h>

#include "NativeSurfaceWrapper.h"

using namespace android;

bool mQuit = false;

void fillRGBA8Buffer(uint8_t* img, int width, int height, int stride, int r, int g, int b) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t* pixel = img + (4 * (y*stride + x));
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
            pixel[3] = 0;
        }
    }
}

int drawNativeSurface(sp<NativeSurfaceWrapper> nativeSurface) {
    status_t err = NO_ERROR;
    int countFrame = 0;
    
    sp<IGraphicBufferProducer> igbProducer;
    nativeSurface->setUpProducer(igbProducer);

    // draw the ANativeWindow
    while(!mQuit) {
        int slot;
        sp<Fence> fence;
        sp<GraphicBuffer> buf;
        
        // 1. dequeue buffer
        igbProducer->dequeueBuffer(&slot, &fence, nativeSurface->width(), nativeSurface->height(),
                                              PIXEL_FORMAT_RGBA_8888, GRALLOC_USAGE_SW_WRITE_OFTEN,
                                              nullptr, nullptr);
        igbProducer->requestBuffer(slot, &buf);

        int waitResult = fence->waitForever("dequeueBuffer_EmptyNative");
        if (waitResult != OK) {
            ALOGE("dequeueBuffer_EmptyNative: Fence::wait returned an error: %d", waitResult);
            break;
        }
        
        // 2. fill the buffer with color
        uint8_t* img = nullptr;
        err = buf->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&img));
        if (err != NO_ERROR) {
            ALOGE("error: lock failed: %s (%d)", strerror(-err), -err);
            break;
        }

        countFrame = (countFrame+1)%3;
        fillRGBA8Buffer(img, nativeSurface->width(), nativeSurface->height(), buf->getStride(),
                        countFrame == 0 ? 255 : 0,
                        countFrame == 1 ? 255 : 0,
                        countFrame == 2 ? 255 : 0);

        err = buf->unlock();
        if (err != NO_ERROR) {
            ALOGE("error: unlock failed: %s (%d)", strerror(-err), -err);
            break;
        }
        
        // 3. queue the buffer to display
        IGraphicBufferProducer::QueueBufferOutput qbOutput;
        IGraphicBufferProducer::QueueBufferInput input(systemTime(), true /* autotimestamp */,
                                                       HAL_DATASPACE_UNKNOWN, {},
                                                       NATIVE_WINDOW_SCALING_MODE_FREEZE, 0,
                                                       Fence::NO_FENCE);
        igbProducer->queueBuffer(slot, input, &qbOutput);

        sleep(1);
    }

    return err;
}

void sighandler(int num) {
    if(num == SIGINT) {
        printf("\nSIGINT: Force to stop !\n");
        mQuit = true;
    }
}

int main() {
    signal(SIGINT, sighandler);

    sp<NativeSurfaceWrapper> nativeSurface(new NativeSurfaceWrapper(String8("NativeSFDemo")));
    drawNativeSurface(nativeSurface);
    return 0;
}