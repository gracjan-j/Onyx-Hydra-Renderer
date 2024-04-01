#include "mesh.h"

#include <iostream>

#include <pxr/imaging/hd/meshUtil.h>
#include <pxr/base/gf/matrix4f.h>

#include <embree4/rtcore_device.h>

#include "renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE
    HdOnyxMesh::HdOnyxMesh(SdfPath const& id)
: HdMesh(id)
{

}


HdDirtyBits HdOnyxMesh::GetInitialDirtyBitsMask() const
{
    // Zakładamy że pierwsza maska oznacza wszystkie możliwe bity
    // nieczystości danych. To pozwala na łatwe wykrycie całkowicie
    // nowych obiektów, w odróżnieniu od obiektów które zostały już
    // raz zsynchronizowane.
    return HdChangeTracker::Clean
    | HdChangeTracker::DirtyPoints
    | HdChangeTracker::DirtyTopology;
}


HdDirtyBits HdOnyxMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // Propagujemy bity bez modyfikacji.
    return bits;
}


void HdOnyxMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    *dirtyBits = GetInitialDirtyBitsMask();
}


void HdOnyxMesh::Sync(
    HdSceneDelegate *sceneDelegate
    , HdRenderParam *renderParam
    , HdDirtyBits *dirtyBits
    , TfToken const &reprToken)
{
    // Pobieramy unikalne ID prima typu Mesh
    auto& primID = GetId();

    // Flaga wskazująca na to czy mesh został zsynchronizowany pierwszy raz.
    bool rebuildMesh = false;

    auto* onyxRenderParam = static_cast<HdOnyxRenderParam*>(renderParam);

    // Warunek będzie prawdziwy tylko dla całkowicie nowych obiektów
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, primID))
    {
        std::cout  << "[hdOnyx] - Utworzono nową geometrię: " << primID.GetString() <<  std::endl;

        // Tworzymy reprezentację obiektu geometrii złożonej z trójkątów w Embree
        // która zostanie powiązana z drzewem BVH sceny.
        m_MeshGeometrySource = rtcNewGeometry(onyxRenderParam->GetEmbreeDevice(), RTC_GEOMETRY_TYPE_TRIANGLE);

        // Nowy obiekt, wskazujemy na konieczność wgrania danych punktów i indeksów punktów.
        rebuildMesh = true;
    }

    // Jeśli mesh nie został jeszcze zainicjalizowany lub buffer punktów uległ zmianie
    if (rebuildMesh || HdChangeTracker::IsPrimvarDirty(*dirtyBits, primID, HdTokens->points))
    {
        // Wnioskujemy o otrzymanie bufora punktów geometrii.
        // Dane zostaną wczytane do tymczasowego bufora.
        const pxr::VtValue& points = GetPrimvar(sceneDelegate, pxr::HdTokens->points);
        if (points.IsHolding<pxr::VtVec3fArray>())
        {
            m_PointArray = points.UncheckedGet<pxr::VtVec3fArray>();
        }

        rtcSetSharedGeometryBuffer(
            m_MeshGeometrySource,
            RTC_BUFFER_TYPE_VERTEX,
            0,
            RTC_FORMAT_FLOAT3,
            m_PointArray.cdata(),
            0,
            sizeof(GfVec3f),
            m_PointArray.size()
        );
    }

    // Jeśli mesh nie został jeszcze zainicjalizowany lub buffer indeksów punktów uległ zmianie
    if (rebuildMesh || HdChangeTracker::IsPrimvarDirty(*dirtyBits, primID, HdTokens->indices))
    {
        pxr::VtIntArray primitiveParams;

        // Pobieramy deskryptor topologii geometrii który posłuży w triangulacji.
        const pxr::HdMeshTopology& meshTopology = GetMeshTopology(sceneDelegate);

        // Tworzymy instancę klasy pomocniczej dla geometri która
        // jest przydatna w triangulacji danych.
        // Dane geometrii mogą być zakodowane w postaci mieszanki trójkątów i czworokątów.
        pxr::HdMeshUtil meshUtil{&meshTopology, GetId()};

        // Dokonujemy triangulacji danych upewniając się że wszystkie czworokąty zostały
        // rozbite na trójkąty. Triangulacji podlega tylko bufor indeksów które odnoszą
        // się do punktów (points) geometrii.
        meshUtil.ComputeTriangleIndices(&m_IndexArray, &primitiveParams);

        rtcSetSharedGeometryBuffer(
            m_MeshGeometrySource,
            RTC_BUFFER_TYPE_INDEX,
            0,
            RTC_FORMAT_UINT3,
            m_IndexArray.cdata(),
            0,
            sizeof(GfVec3i),
            m_IndexArray.size()
        );

        auto err = rtcGetDeviceError(onyxRenderParam->GetEmbreeDevice());

        if (err == RTC_ERROR_NONE)
        {
            bool a;
        }
    }

    // Jeśli mesh nie został zainicjalizowany lub jego transformacja uległa zmianie
    if (rebuildMesh || HdChangeTracker::IsTransformDirty(*dirtyBits, primID))
    {
        pxr::GfMatrix4f meshTransformMatrix = GfMatrix4f(sceneDelegate->GetTransform(primID));

        rtcSetGeometryTransform(
            m_MeshGeometrySource,
            0,
            RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,
            meshTransformMatrix.GetArray()
        );

        auto err = rtcGetDeviceError(onyxRenderParam->GetEmbreeDevice());

        if (err == RTC_ERROR_NONE)
        {
            bool a;
        }

    }

    // Dane zostały uzupełnione, wnioskujemy o zbudowanie obiektu geometrii Embree.
    // CommitGeometry musi zostać wywołane przed użyciem obiektu w scenie Embree.
    rtcCommitGeometry(m_MeshGeometrySource);

    // Dodajemy geometrię do sceny.
    m_MeshAttachmentID = onyxRenderParam->GetRendererHandle()->AddTriangleGeometrySource(m_MeshGeometrySource);

    // Dokonaliśmy niezbędnej synchronizacji danych na których nam zależy.
    // Oznaczamy prim jako wolny od zmian.
    // W przypadku modyfikacji parametrów, USD zadba o ustawienie wymaganych bitów.
    *dirtyBits = HdChangeTracker::Clean;
}

PXR_NAMESPACE_CLOSE_SCOPE
