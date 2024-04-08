#pragma once

#include <embree4/rtcore_ray.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/matrix3f.h>

// TODO: Przenieś do OnyxRenderer.
#include "../../hdOnyx/include/mesh.h"

namespace Onyx
{
    class OnyxHelper
    {
    public:

        /**
        * Metoda pomocnicza służąca do interpolacji danych na podstawie współrzędnych barycentrycznych trójkąta.
        * @param UV Współrzedne barycentryczne trójkąta
        * @param V0 Atrybut wierzchołka pierwszego
        * @param V1 Atrybut wierzchołka drugiego
        * @param V2 Atrybut wierzchołka trzeciego
        * @return Atrybut po interpolacji.
        */
        static pxr::GfVec3f InterpolateWithBarycentricCoordinates(
            const pxr::GfVec2f& UV,
            const pxr::GfVec3f& V0,
            const pxr::GfVec3f& V1,
            const pxr::GfVec3f& V2
        );


        /**
        * Metoda pomocnicza służąca do ewaluacji wektora normalnego na podstawie warunków intersekcji.
        * Jeśli intersekcja zwraca prawidłową strukturę pomocniczą instancji, sprawdzamy czy udostępnia ona
        * wskaźnik do bufora wektorów normalnych. Jeśli tak, używamy wektora z bufora, w innym przypadku
        * korzystamy z wektora geometrycznego powierzchni.
        *
        * @param rayHitData Promień który otrzymał poprawne dane intersekcji.
        * @param embreeScene Scena z którą promień testował intersekcję.
        * @return Wektor normalny powierzchni w globalnej przestrzeni sceny (world-space).
        */
        static pxr::GfVec3f EvaluateHitSurfaceNormal(
            const RTCRayHit& rayHitData,
            const RTCScene& embreeScene
        );


        /**
         * Metoda pomocnicza służąca do tworzenia promienia wychodzącego z kamery (primary ray).
         *
         * @param pixelOffsetX Przesunięcie piksela w osi X (index piksela w kontekście szerokości)
         * @param pixelOffsetY Przesunięcie piksela w osi Y (index piksela w kontekście wysokości)
         * @param maxX Limit osi X (rozdzielczość X - szerokość)
         * @param maxY Limit osi Y (rozdzielczość Y - wysokość)
         * @param inverseProjectionMatrix Odwrotna macierz projekcji kamery
         * @param inverseViewMatrix Odwrotna macierz transformacji widoku kamery
         * @return Zainicjalizowany promień kamery dla Embree, gotowy do testu intersekcji.
         */
        static RTCRayHit GeneratePrimaryRay(
            const float& pixelOffsetX, const float& pixelOffsetY,
            const float& maxX, const float& maxY,
            const pxr::GfMatrix4d& inverseProjectionMatrix, const pxr::GfMatrix4d& inverseViewMatrix
        );


        /**
         * Metoda pomocnicza służąca do tworzenia promienia odbicia na podstawie przekazanego kierunku odbicia
         * oraz punktu kolizji.
         *
         * @param bounceDirection Znormalizowany kierunek odbicia w world-space.
         * @param hitPosition Punkt startowy nowego promienia w world-space.
         * @param hitNormal Wektor normalny powierzchni punktu startowego.
         * @return Zainicjalizowany promień odbicia dla Embree, gotowy do testu intersekcji.
         * @note Początek startowy promienia jest przesuwany zgodnie z wektorem normalnym
         *       w celu uniknięcia intersekcji z powierzchnią punktu startowego.
         */
        static RTCRayHit GenerateBounceRay(
            const pxr::GfVec3f& bounceDirection,
            const pxr::GfVec3f& hitPosition,
            const pxr::GfVec3f& hitNormal
        );


        static pxr::GfMatrix3f GenerateOrthogonalFrameInZ(pxr::GfVec3f zAxis);
    };

}