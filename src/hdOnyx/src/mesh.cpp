#include "mesh.h"

#include <iostream>

#include <pxr/imaging/hd/meshUtil.h>
#include <pxr/imaging/hd/vtBufferSource.h>

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

    bool topologyChanged = false;

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

        topologyChanged = true;
    }

    // Pobieramy deskryptor topologii geometrii który posłuży w triangulacji.
    const pxr::HdMeshTopology& meshTopology = GetMeshTopology(sceneDelegate);

    // Tworzymy instancę klasy pomocniczej dla geometri która
    // jest przydatna w triangulacji danych.
    // Dane geometrii mogą być zakodowane w postaci mieszanki trójkątów i czworokątów.
    pxr::HdMeshUtil meshUtil{&meshTopology, GetId()};

    // Jeśli mesh nie został jeszcze zainicjalizowany lub buffer indeksów punktów uległ zmianie
    if (rebuildMesh || HdChangeTracker::IsPrimvarDirty(*dirtyBits, primID, HdTokens->indices))
    {
        pxr::VtIntArray primitiveParams;

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

        topologyChanged = true;
    }

    // Jeśli mesh nie został jeszcze zainicjalizowany lub buffer gładkich wektorów normalnych
    // został zmieniony. Wygładzone wektory normalne są opcjonalne, jeśli są obecne, silnik ich użyje.
    if (rebuildMesh || HdChangeTracker::IsPrimvarDirty(*dirtyBits, primID, HdTokens->normals))
    {
        // Pobieramy dane wektorów normalnych. Primvar - Primitive Variable - Dodatkowe dane dołączone
        // do figur z których zbudowana jest geometria.
        pxr::VtValue smoothNormalPrimvar = GetPrimvar(sceneDelegate, pxr::HdTokens->normals);

        // Podobnie jak w przypadku indesków punktów, wektory mogą być zdefiniowane dla mieszanki czworokątów
        // jak i trójkątów, ponownie używamy klasy pomocniczej USD do uzyskania wektorów po triangulacji.
        pxr::HdVtBufferSource inputBufferNormals{pxr::HdTokens->normals, smoothNormalPrimvar};
        pxr::VtValue triangulationOutput;

        bool normalTriangulationValid = meshUtil.ComputeTriangulatedFaceVaryingPrimvar(
            inputBufferNormals.GetData(),
            inputBufferNormals.GetNumElements(),
            inputBufferNormals.GetTupleType().type,
            &triangulationOutput);

        // Jeśli dane istnieją i mają poprawny format.
        if (normalTriangulationValid && triangulationOutput.IsHolding<pxr::VtVec3fArray>())
        {
            m_SmoothNormalArray = triangulationOutput.UncheckedGet<pxr::VtVec3fArray>();
        }

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

        topologyChanged = true;
    }

    // Jeśli topologia geometrii uległa zmianie, musimy wywołać budowę triangle BVH geometrii.
    if (topologyChanged)
    {
        // Dane zostały uzupełnione, wnioskujemy o zbudowanie obiektu geometrii Embree.
        // CommitGeometry musi zostać wywołane przed utworzeniem prymitywnego obiektu geometrii.
        rtcCommitGeometry(m_MeshGeometrySource);

        m_MeshRTAS = rtcNewScene(onyxRenderParam->GetEmbreeDevice());

        // Podpinamy deskryptor geometrii do prymitywnego obiektu geometrii
        rtcAttachGeometry(m_MeshRTAS, m_MeshGeometrySource);

        // Wnioskujemy o zbudowanie prymitywnego obiektu geometrii
        // Embree którego użyjemy do stworzenia instancji w scenie.
        rtcCommitScene(m_MeshRTAS);
    }

    // Sprawdzamy czy materiał powiązany z geometrią został zmieniony.
    // Powiązanie materiału oraz geometrii odbywa sięza pomocą ścieżki do rprima typu "Material".
    bool isMaterialBindingDirty = *dirtyBits & HdChangeTracker::DirtyMaterialId != 0;

    // Jeśli mesh nie został zainicjalizowany lub jego transformacja uległa zmianie
    if (rebuildMesh || topologyChanged || isMaterialBindingDirty
        || HdChangeTracker::IsTransformDirty(*dirtyBits, primID))
    {
        // Jeśli instancja została wcześniej podpięta pod główną scenę
        if (m_InstanceAttachmentID.has_value())
        {
            // Dokonujemy odpięcia instancji geometrii od głównej sceny silnika w celu aktualizacji.
            onyxRenderParam->GetRendererHandle()->DetachGeometryFromScene(m_InstanceAttachmentID.value());

            // Jednocześnie zaznaczamy że instancja została odpięta i nie ma już przypisanego identyfikatora.
            m_InstanceAttachmentID = std::nullopt;
        }

        // Pobieramy ścieżkę do materiału przypisaną geometrii.
        SdfPath materialPath = sceneDelegate->GetMaterialId(primID);
        // Szukamy materiału "po ścieżce" w mapie materialów silnika.
        // Silnik zwróci index materiału w buforze materiałów.
        auto matInBufferID = onyxRenderParam->GetRendererHandle()->GetIndexOfMaterialByPath(materialPath);

        // Pobieramy aktualną transformację obiektu i uzupełniamy nią strukturę danych instancji.
        m_InstanceData = {
            .TransformMatrix = GfMatrix4f(sceneDelegate->GetTransform(primID)),
            .SmoothNormalsArray = m_SmoothNormalArray.has_value()
                ? &(m_SmoothNormalArray.value())
                : nullptr,
            .MaterialIndexInBuffer = matInBufferID
        };

        // Tworzymy nową geometrię typu - instance
        // Korzystamy w ten sposób z możliwości utworzenia wirtualnej kopii bazowej geometrii
        // z własnym przekształceniem, zamiast modyfikacji bazowej geometrii transformacją.
        m_MeshInstanceSource = rtcNewGeometry(onyxRenderParam->GetEmbreeDevice(), RTC_GEOMETRY_TYPE_INSTANCE);

        // Ustawiamy źródło instancji - bazowy obiekt geometrii
        rtcSetGeometryInstancedScene(m_MeshInstanceSource, m_MeshRTAS);
        rtcSetGeometryTimeStepCount(m_MeshInstanceSource, 1);

        // Wywołanie funkcji SetGeometryTransform jest możliwe tylko i wyłącznie na
        // instancjach. Korzystająć z prymitywnego typu geometrii - triangle w Embree
        // możemy osiągnąć poprawną transformację, transformując wierzchołki (punkty) geometrii.
        // Jednak, lepszym rozwiązaniem jest wykorzystanie instancingu.
        rtcSetGeometryTransform(
            m_MeshInstanceSource,
            0,
            RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,
            m_InstanceData.TransformMatrix.GetArray()
        );

        // Powiązujemy małą strukturę z instancją. Podczas testu intersekcji,
        // możemy otrzymać poniższy wskaźnik do struktury powiązany z instancją.
        rtcSetGeometryUserData(m_MeshInstanceSource, &m_InstanceData);

        rtcCommitGeometry(m_MeshInstanceSource);
    }

    // Jeśli instancja nie jest aktualnie powiązana z główną sceną silnika.
    if (!m_InstanceAttachmentID.has_value())
    {
        // Powiązujemy instancę z główną sceną silnika.
        m_InstanceAttachmentID = onyxRenderParam->GetRendererHandle()->AttachGeometryToScene(m_MeshInstanceSource);
    }

    // Dokonaliśmy niezbędnej synchronizacji danych na których nam zależy.
    // Oznaczamy prim jako wolny od zmian.
    // W przypadku modyfikacji parametrów, USD zadba o ustawienie wymaganych bitów.
    *dirtyBits = HdChangeTracker::Clean;
}

PXR_NAMESPACE_CLOSE_SCOPE
