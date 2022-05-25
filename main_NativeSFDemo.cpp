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
    ANativeWindowBuffer *nativeBuffer = nullptr;
    ANativeWindow* nativeWindow = nativeSurface->getSurface().get();

    // 1. connect the ANativeWindow as a CPU client. Buffers will be queued after being filled using the CPU
    err = native_window_api_connect(nativeWindow, NATIVE_WINDOW_API_CPU);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to native_window_api_connect\n");
        return EXIT_FAILURE;
    }

    // 2. set the ANativeWindow dimensions
    err = native_window_set_buffers_user_dimensions(nativeWindow, nativeSurface->width(), nativeSurface->height());
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to native_window_set_buffers_user_dimensions\n");
        return EXIT_FAILURE;
    }

    // 3. set the ANativeWindow format
    err = native_window_set_buffers_format(nativeWindow, PIXEL_FORMAT_RGBX_8888);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to native_window_set_buffers_format\n");
        return EXIT_FAILURE;
    }

    // 4. set the ANativeWindow usage
    err = native_window_set_usage(nativeWindow, GRALLOC_USAGE_SW_WRITE_OFTEN);
    if (err != NO_ERROR) {
        ALOGE("native_window_set_usage failed: %s (%d)", strerror(-err), -err);
        return err;
    }

    // 5. set the ANativeWindow scale mode
    err = native_window_set_scaling_mode(nativeWindow, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
    if (err != NO_ERROR) {
        ALOGE("native_window_set_scaling_mode failed: %s (%d)", strerror(-err), -err);
        return err;
    }

    // 6. set the ANativeWindow permission to allocte new buffer, default is true
    static_cast<Surface*>(nativeWindow)->getIGraphicBufferProducer()->allowAllocation(true);

    // 7. set the ANativeWindow buffer count
    int numBufs = 0;
    int minUndequeuedBufs = 0;

    err = nativeWindow->query(nativeWindow,
            NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeuedBufs);
    if (err != NO_ERROR) {
        ALOGE("error: MIN_UNDEQUEUED_BUFFERS query "
                "failed: %s (%d)", strerror(-err), -err);
        goto handle_error;
    }

    numBufs = minUndequeuedBufs + 1;
    err = native_window_set_buffer_count(nativeWindow, numBufs);
    if (err != NO_ERROR) {
        ALOGE("error: set_buffer_count failed: %s (%d)", strerror(-err), -err);
        goto handle_error;
    }

    // 8. draw the ANativeWindow
    while(!mQuit) {
        // 9. dequeue a buffer
        int releaseFenceFd = -1;
        err = nativeWindow->dequeueBuffer(nativeWindow, &nativeBuffer, &releaseFenceFd);
        if (err != NO_ERROR) {
            ALOGE("error: dequeueBuffer failed: %s (%d)",
                    strerror(-err), -err);
            break;
        }

        // 10. make sure really control the dequeued buffer
        sp<Fence> releaseFence(new Fence(releaseFenceFd));
        int waitResult = releaseFence->waitForever("dequeueBuffer_EmptyNative");
        if (waitResult != OK) {
            ALOGE("dequeueBuffer_EmptyNative: Fence::wait returned an error: %d", waitResult);
            break;
        }

        sp<GraphicBuffer> buf(GraphicBuffer::from(nativeBuffer));

        // 11. Fill the buffer with black
        uint8_t* img = nullptr;
        err = buf->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&img));
        if (err != NO_ERROR) {
            ALOGE("error: lock failed: %s (%d)", strerror(-err), -err);
            break;
        }

        //12. Draw the window
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

        // 13. queue the buffer to display
        int acquireFenceFd = -1;
        err = nativeWindow->queueBuffer(nativeWindow, buf->getNativeBuffer(), acquireFenceFd);
        if (err != NO_ERROR) {
            ALOGE("error: queueBuffer failed: %s (%d)", strerror(-err), -err);
            break;
        }

        nativeBuffer = nullptr;
        sleep(1);
    }

handle_error:
    // 14. cancel buffer
    if (nativeBuffer != nullptr) {
        nativeWindow->cancelBuffer(nativeWindow, nativeBuffer, -1);
        nativeBuffer = nullptr;
    }

    // 15. Clean up after success or error.
    err = native_window_api_disconnect(nativeWindow, NATIVE_WINDOW_API_CPU);
    if (err != NO_ERROR) {
        ALOGE("error: api_disconnect failed: %s (%d)", strerror(-err), -err);
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