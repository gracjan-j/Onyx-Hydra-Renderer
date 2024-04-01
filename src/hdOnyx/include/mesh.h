#pragma once

#include <embree4/rtcore_geometry.h>

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdOnyxMesh final : public HdMesh
{
public:
    HdOnyxMesh(SdfPath const& id);

    ~HdOnyxMesh() override = default;

    // Metoda zwraca informacje na temat danych które powinny
    // zostać pobrane przy pierwszej synchronizacji ze sceną.
    // Możemy wykluczyć dane które nie są dla nas interesujące.
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    // Metoda jest wywoływana przez subsystem Hydra w OpenUSD dla
    // primów które uległy zmianie. Umożliwia (pełne/selektywne) pobieranie
    // nowych danych dla konkretnego prima na podstawie flagi czystości danych.
    void Sync(
        HdSceneDelegate* sceneDelegate
        , HdRenderParam* renderParam
        , HdDirtyBits* dirtyBits
        , TfToken const &reprToken) override;

protected:

    void _InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits) override;

    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    HdOnyxMesh(const HdOnyxMesh&) = delete;
    HdOnyxMesh &operator =(const HdOnyxMesh&) = delete;

private:
    RTCGeometry m_MeshGeometrySource;
    uint m_MeshAttachmentID = 0;

    // Bufor punktów (points / vertices) geometrii.
    VtVec3fArray m_PointArray;

    // Bufor ztriangulowanych indeksów (indices) punktów geometrii
    pxr::VtVec3iArray m_IndexArray;
};


PXR_NAMESPACE_CLOSE_SCOPE
