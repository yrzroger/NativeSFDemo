/*
 * Copyright (C) 2021 The Android Open Source Project
 */

#ifndef SURFACE_WRAPPER_H
#define SURFACE_WRAPPER_H

#include <gui/Surface.h>
#include <gui/SurfaceControl.h>
#include <system/window.h>
#include <utils/RefBase.h>

namespace android {

class NativeSurfaceWrapper : public RefBase {
public:
    NativeSurfaceWrapper(const String8& name, uint32_t layerStack = 0);
    virtual ~NativeSurfaceWrapper() {}

    virtual void onFirstRef();

    // Retrieves a handle to the window.
    sp<ANativeWindow>  getSurface() const;

    int width() { return mWidth; }
    int height() { return mHeight; }

private:
    DISALLOW_COPY_AND_ASSIGN(NativeSurfaceWrapper);
    ui::Size limitSurfaceSize(int width, int height) const;

    sp<SurfaceControl> mSurfaceControl;
    int mWidth;
    int mHeight;
    String8 mName;
    uint32_t mLayerStack;
};

} // namespace android

#endif // SURFACE_WRAPPER_H
