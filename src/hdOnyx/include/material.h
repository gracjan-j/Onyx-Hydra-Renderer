#pragma once

#include <pxr/pxr.h>
#include <pxr/imaging/hd/material.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdOnyxMaterial : public HdMaterial
{
public:
    HdOnyxMaterial(SdfPath const& id);
    ~HdOnyxMaterial() override = default;

    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;


protected:
    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    /**
     * Indeks materiału przypisany przez renderer w buforze materiałów sceny.
     */
    uint m_MaterialBufferID;
};

PXR_NAMESPACE_CLOSE_SCOPE
