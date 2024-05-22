/*
 * Copyright (C) 2021 The Android Open Source Project
 */

#define LOG_TAG "NativeSurfaceWrapper"

#include <android-base/properties.h>
#include <android/gui/ISurfaceComposerClient.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayState.h>
#include <utils/Log.h>

#include "NativeSurfaceWrapper.h"

namespace android {

NativeSurfaceWrapper::NativeSurfaceWrapper(const String8& name, uint32_t layerStack) 
    : mName(name), mLayerStack(layerStack) {}

void NativeSurfaceWrapper::onFirstRef() {
    // create & init SurfaceComposerClient
    sp<SurfaceComposerClient> surfaceComposerClient = new SurfaceComposerClient;
    status_t err = surfaceComposerClient->initCheck();
    if (err != OK) {
        ALOGD("SurfaceComposerClient::initCheck error: %#x\n", err);
        return;
    }
    // get ID for any displays
    const std::vector<PhysicalDisplayId> ids = SurfaceComposerClient::getPhysicalDisplayIds();
    if (ids.empty()) {
        ALOGE("Failed to get ID for any displays\n");
        return;
    }
    // get display information
    sp<IBinder> displayToken = nullptr;
    for (const auto id : ids) {
        displayToken = SurfaceComposerClient::getPhysicalDisplayToken(id);
        if (displayToken != nullptr) {
            ui::DisplayState ds;
            err = SurfaceComposerClient::getDisplayState(displayToken, &ds);
            if(err == OK && ds.layerStack.id == mLayerStack) {
                break;
            }
        }
    }

    // default -- display 0 is used
    if (displayToken == nullptr) {
        displayToken = SurfaceComposerClient::getPhysicalDisplayToken(ids.front());
        mLayerStack = 0;
        if (displayToken == nullptr) {
            ALOGE("Failed to getPhysicalDisplayToken");
            return;
        }
    }
    // get display's resolution
    ui::DisplayMode displayMode;
    err = SurfaceComposerClient::getActiveDisplayMode(displayToken, &displayMode);
    if (err != OK)
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
            .setLayerStack(surfaceControl, ui::LayerStack::fromValue(mLayerStack))
            .apply();

    mWidth = resolution.getWidth();
    mHeight = resolution.getHeight();
    mSurfaceControl = surfaceControl;
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
