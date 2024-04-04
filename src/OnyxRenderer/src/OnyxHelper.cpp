#include "OnyxHelper.h"

#include <embree4/rtcore_scene.h>


pxr::GfVec3f OnyxHelper::InterpolateWithBarycentricCoordinates(
    const pxr::GfVec2f& UV,
    const pxr::GfVec3f& V0,
    const pxr::GfVec3f& V1,
    const pxr::GfVec3f& V2)
{
    return (1.0f - UV.GetArray()[0] -UV.GetArray()[1])
    * V0 + UV.GetArray()[0]
    * V1 + UV.GetArray()[1]
    * V2;
}


pxr::GfVec3f OnyxHelper::EvaluateHitSurfaceNormal(
    const RTCRayHit& hitRayData,
    const RTCScene& embreeScene)
{
    pxr::GfVec3f hitLocalNormal;

    // Pobieramy strukturę pomocniczą powiązaną z instancją na podstawie identyfikatora instancji
    // w scenie, który otrzymujemy przy intersekcji.
    auto* hitInstanceData = static_cast<pxr::HdOnyxInstanceData*>(
        rtcGetGeometryUserData(
            // Identyfikator instancji zakłada jedno-poziomowy instancing ([0]).
            // Zgodne z założeniem w HdOnyxMesh który tworzy geometrię.
            rtcGetGeometry(embreeScene, hitRayData.hit.instID[0])
        )
    );

    // Identyfikator uderzonego trójkąta instancji.
    uint primitiveID = hitRayData.hit.primID;

    // Jeśli mamy dostęp do bufora wektorów normalnych, używamy go do otrzymania "wygładzonego" wektora.
    if (hitInstanceData->SmoothNormalsArray && hitInstanceData->SmoothNormalsArray->size() > primitiveID)
    {
        // Każdy punkt ma swój odpowiednik w buforach primvar (Primitive-variable).
        // Używamy primID (index trójkąta) do otrzymania wektora dla każdego punktu uderzonego trójkąta.
        // Obliczamy bazowy offset trójkąta w buforze (3 * index - wierzchołek podstawowy)
        // i uzyskujemy pozostałe dwa wierzchołki.
        auto N0 = hitInstanceData->SmoothNormalsArray->cdata()[3 * primitiveID + 0];
        auto N1 = hitInstanceData->SmoothNormalsArray->cdata()[3 * primitiveID + 1];
        auto N2 = hitInstanceData->SmoothNormalsArray->cdata()[3 * primitiveID + 2];

        // Dokonujemy interpolacji danych na podstawie współrzędnych barycentrycznych
        // których dostarcza Embree dla uderzonego trójkąta.
        hitLocalNormal = InterpolateWithBarycentricCoordinates(
            {hitRayData.hit.u, hitRayData.hit.v},
            N0, N1, N2
        );

        hitLocalNormal.Normalize();
    }
    else
    {
        // Musimy pobrać wektor geometryczny (obliczony przez Embree na podstawie wierzchołków trójkąta).
        hitLocalNormal = { hitRayData.hit.Ng_x, hitRayData.hit.Ng_y, hitRayData.hit.Ng_z };

        // Normalizujemy in-place.
        hitLocalNormal.Normalize();
    }

    // Obliczony wektor wymaga przekształcenia. Dane w buforze oraz obliczony wektor geometryczny
    // są zdefiniowane w object-space (local). Aby otrzymać prawidłowy wektor normalny musimy
    // obliczyć jego przekształcenie na podstawie transformacji instancji.
    // Używamy macierzy transformacji instancji do transformacji wektora (dir - ignorujemy translację) do world-space.
    pxr::GfVec3f hitWorldNormal = hitInstanceData->TransformMatrix.TransformDir(hitLocalNormal);
    hitWorldNormal.Normalize();

    return hitWorldNormal;
}


