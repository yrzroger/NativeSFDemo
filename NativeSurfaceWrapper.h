/*
 * Copyright (C) 2021 The Android Open Source Project
 */

#ifndef SURFACE_WRAPPER_H
#define SURFACE_WRAPPER_H

#include <gui/BLASTBufferQueue.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <gui/SurfaceControl.h>
#include <system/window.h>
#include <utils/RefBase.h>

namespace android {

class NativeSurfaceWrapper : public RefBase {
public:
    NativeSurfaceWrapper(const String8& name);
    virtual ~NativeSurfaceWrapper() {}

    virtual void onFirstRef();

    void setUpProducer(sp<IGraphicBufferProducer>& producer);

    int width() { return mWidth; }
    int height() { return mHeight; }

private:
    DISALLOW_COPY_AND_ASSIGN(NativeSurfaceWrapper);
    ui::Size limitSurfaceSize(int width, int height) const;

    sp<BLASTBufferQueue> mBlastBufferQueue;
    int mWidth;
    int mHeight;
    String8 mName;
};

} // namespace android

#endif // SURFACE_WRAPPER_H
