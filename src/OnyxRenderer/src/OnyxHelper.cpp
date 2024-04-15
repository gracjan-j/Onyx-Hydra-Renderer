#include "OnyxHelper.h"

#include <embree4/rtcore_scene.h>

using namespace Onyx;


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


RTCRayHit OnyxHelper::GeneratePrimaryRay(
    const float& pixelOffsetX, const float& pixelOffsetY,
    const float& maxX, const float& maxY,
    const pxr::GfMatrix4d& inverseProjectionMatrix, const pxr::GfMatrix4d& inverseViewMatrix,
    const pxr::GfVec2f uniform2D)
{
    // Obliczamy Normalised Device Coordinates dla aktualnego piksela
    // NDC reprezentują pozycję pikela w screen-space.
    pxr::GfVec3f NDC{
        2.0f * (float(pixelOffsetX + uniform2D[0]) / maxX) - 1.0f,
        2.0f * (float(pixelOffsetY + uniform2D[1]) / maxY) - 1.0f,
        -1};

    // Przechodzimy z NDC (screen-space) do clip-space za pomocą odwrotnej projekcji
    // perspektywy wirtualnej kamery. -1 w NDC będzie odpowiadało bliskiej płaszczyźnie projekcji.
    const pxr::GfVec3f nearPlaneProjection = inverseProjectionMatrix.Transform(NDC);

    // Lokalnie, kamera znajduje się w centrum układu współrzednych
    pxr::GfVec3f rayOrigin = inverseViewMatrix.Transform(pxr::GfVec3f(0.0, 0.0, 0.0));
    pxr::GfVec3f rayDir = inverseViewMatrix.TransformDir(nearPlaneProjection).GetNormalized();

    return {.ray = {.org_x = rayOrigin[0],
                    .org_y = rayOrigin[1],
                    .org_z = rayOrigin[2],
                    .dir_x = rayDir[0],
                    .dir_y = rayDir[1],
                    .dir_z = rayDir[2],
                    // Nie ustanawiamy minimalnego limitu intersekcji.
                    .tnear = 0.0,
                    // Maksymalnym limitem teoretycznie jest limit precyzji typu danych
                    .tfar = std::numeric_limits<float>::infinity(),
                    // Nie używamy maskowania. Uwzględniamy wszystkie obiekty sceny
                    .mask = UINT_MAX,
                    // Nie korzystamy z rozmycia spowodowanego ruchem
                    .time = 0.0},
            .hit = {.geomID = RTC_INVALID_GEOMETRY_ID}};
}


RTCRayHit OnyxHelper::GenerateBounceRay(
    const pxr::GfVec3f& bounceDirection,
    const pxr::GfVec3f& hitPosition,
    const pxr::GfVec3f& hitNormal)
{
    // Przesuwamy punkt startowy w celu uniknięcia self-intersection (intersekcji z geometrią w którą ostatnio
    // uderzył promień).
    auto offsetHitPosition = hitPosition + (hitNormal * 0.001);
    return {
        .ray = {
            .org_x = offsetHitPosition[0],
            .org_y = offsetHitPosition[1],
            .org_z = offsetHitPosition[2],
            .dir_x = bounceDirection[0],
            .dir_y = bounceDirection[1],
            .dir_z = bounceDirection[2],
            // Nie ustanawiamy minimalnego limitu intersekcji.
            .tnear = 0.0,
            // Maksymalnym limitem teoretycznie jest limit precyzji typu danych
            .tfar = std::numeric_limits<float>::infinity(),
            // Nie używamy maskowania. Uwzględniamy wszystkie obiekty sceny
            .mask = UINT_MAX,
            // Nie korzystamy z rozmycia spowodowanego ruchem
            .time = 0.0},

        .hit = {
            .geomID = RTC_INVALID_GEOMETRY_ID
        }
    };
}


pxr::GfMatrix3f OnyxHelper::GenerateOrthogonalFrameInZ(pxr::GfVec3f zAxis)
{
    // Przygotowujemy macierz która będzie zaweirała końcową transformację.
    pxr::GfMatrix3f orthogonalFrame;
    orthogonalFrame.SetIdentity();

    pxr::GfVec3f xAxis;
    pxr::GfVec3f yAxis;

    // Obliczamy osie prostopadłe aby obliczyć lokalny układ współrzędnych gdzie
    // wektor normalny jest osią Z.
    pxr::GfBuildOrthonormalFrame(zAxis, &xAxis, &yAxis);

    // Tworzymy macierz na podstawie przygotowanych osi tworząc w ten sposób
    // transformację local-world space.
    orthogonalFrame.SetColumn(0, xAxis.GetNormalized());
    orthogonalFrame.SetColumn(1, yAxis.GetNormalized());
    orthogonalFrame.SetColumn(2, zAxis);

    return orthogonalFrame;
}