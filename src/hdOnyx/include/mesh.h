#pragma once

#include <embree4/rtcore_geometry.h>

#include <pxr/pxr.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/base/gf/matrix4f.h>

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

    // Struktura przyspieszenia intersekcji Embree zbudowana na podstawie
    // punktów oraz indeksów geometrii. Aby uniknąć transformacji
    // bufora punktów przez transformację obiektu w scenie
    // wykorzystujemy tzw. instancing.
    // Tworzymy strukturę którą później wykorzystamy do stworzenia instancji.
    // Scena używana przez silnik używa tylko instancji geometrii.
    RTCScene m_MeshRTAS;

    // Desktryptor geometrii dla której powinna zostać zbuowana
    // struktura przyspieszenia intersekcji.
    // (RTAS / BVH - Ray Tracing Acceleration Structure / Bounding Volume Hierarchy)
    // Zawiera bufory punktów oraz indeksów trójkątów budujących mesh.
    RTCGeometry m_MeshGeometrySource;

    // Deskryptor instancji w strukturze przyspieszenia intersekcji
    // która zostaje powiązana z główną sceną silnika.
    RTCGeometry m_MeshInstanceSource;

    // Indeks pod jakim instancja mesha została powiązana ze sceną główną silnika (ID instancji).
    std::optional<uint> m_InstanceAttachmentID;

    // Bufor punktów (points / vertices) geometrii.
    VtVec3fArray m_PointArray;

    // Bufor ztriangulowanych indeksów (indices) punktów geometrii
    pxr::VtVec3iArray m_IndexArray;

    // Transformacja geometri (uwzględnia wszystkie transformacje nadrzędne w hierarchii sceny)
    pxr::GfMatrix4f m_InstanceTransformMatrix;
};


PXR_NAMESPACE_CLOSE_SCOPE
