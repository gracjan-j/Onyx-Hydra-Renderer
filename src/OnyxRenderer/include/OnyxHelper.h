#pragma once

#include <embree4/rtcore_ray.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec2f.h>

// TODO: Przenieś do OnyxRenderer.
#include "../../hdOnyx/include/mesh.h"

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

};

