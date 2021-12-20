/*
 * Copyright (C) 2021 The Android Open Source Project
 */

#define LOG_TAG "NativeSurfaceWrapper"

#include <android-base/properties.h>
#include <gui/ISurfaceComposerClient.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <utils/Log.h>

#include "NativeSurfaceWrapper.h"

namespace android {

NativeSurfaceWrapper::NativeSurfaceWrapper(const String8& name) 
    : mName(name) {}

void NativeSurfaceWrapper::onFirstRef() {
    sp<SurfaceComposerClient> surfaceComposerClient = new SurfaceComposerClient;
    status_t err = surfaceComposerClient->initCheck();
    if (err != NO_ERROR) {
        ALOGD("SurfaceComposerClient::initCheck error: %#x\n", err);
        return;
    }

    // Get main display parameters.
    sp<IBinder> displayToken = SurfaceComposerClient::getInternalDisplayToken();
    if (displayToken == nullptr)
        return;

    ui::DisplayMode displayMode;
    const status_t error =
            SurfaceComposerClient::getActiveDisplayMode(displayToken, &displayMode);
    if (error != NO_ERROR)
        return;

    ui::Size resolution = displayMode.resolution;
    resolution = limitSurfaceSize(resolution.width, resolution.height);
    // create the native surface
    sp<SurfaceControl> surfaceControl = surfaceComposerClient->createSurface(mName, resolution.getWidth(), 
                                                                             resolution.getHeight(), PIXEL_FORMAT_RGBA_8888,
                                                                             ISurfaceComposerClient::eFXSurfaceBufferState,
                                                                             /*parent*/ nullptr);

    SurfaceComposerClient::Transaction{}
            .setLayer(surfaceControl, std::numeric_limits<int32_t>::max())
            .show(surfaceControl)
            .apply();

    mSurfaceControl = surfaceControl;
    mWidth = resolution.getWidth();
    mHeight = resolution.getHeight();
}

sp<ANativeWindow> NativeSurfaceWrapper::getSurface() const {
    sp<ANativeWindow> anw = mSurfaceControl->getSurface();
    return anw;
}

ui::Size NativeSurfaceWrapper::limitSurfaceSize(int width, int height) const {
    ui::Size limited(width, height);
    bool wasLimited = false;
    const float aspectRatio = float(width) / float(height);

    int maxWidth = android::base::GetIntProperty("ro.surface_flinger.max_graphics_width", 0);
    int maxHeight = android::base::GetIntProperty("ro.surface_flinger.max_graphics_height", 0);

    if (maxWidth != 0 && width > maxWidth) {
        limited.height = maxWidth / aspectRatio;
        limited.width = maxWidth;
        wasLimited = true;
    }
    if (maxHeight != 0 && limited.height > maxHeight) {
        limited.height = maxHeight;
        limited.width = maxHeight * aspectRatio;
        wasLimited = true;
    }
    SLOGV_IF(wasLimited, "Surface size has been limited to [%dx%d] from [%dx%d]",
             limited.width, limited.height, width, height);
    return limited;
}

} // namespace android
