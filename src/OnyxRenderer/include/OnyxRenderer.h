#pragma once

#include <memory>
#include <embree4/rtcore.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/usd/sdf/path.h>

#include "Material.h"
#include "RenderArgument.h"

#include "../../hdOnyx/include/mesh.h"


namespace Onyx
{

    class OnyxRenderer
    {
    public:

        OnyxRenderer();
        ~OnyxRenderer();

        /**
         * Metoda wywoływana przez wątek renderujący jako entry-point procesu renderowania.
         * @param renderThread Wątek renderujący który wykonuje proces renderowania.
         */
        void MainRenderingEntrypoint(pxr::HdRenderThread* renderThread);

        bool RenderAllAOV();

        void SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix);


        /**
         * Metoda podpinająca geometrię do sceny Embree silnika.
         * @param geometrySource Geometria do powiązania ze sceną
         * @return Indeks powiązania obiektu ze sceną. Przydatny w przypadku konieczności cofnięcia operacji.
         */
        uint AttachGeometryToScene(const RTCGeometry& geometrySource);


        uint AttachLightInstanceToScene(
            pxr::HdOnyxInstanceData* instanceData,
            const pxr::GfVec3f& totalEmissionPower);


        /**
         * Metoda dodająca lub aktualizująca dane materiału w silniku.
         * @param diffuseColor Kolor materiału matowego (idealny rozpraszacz)
         * @param IOR Indeks załamania materiału.
         * @param materialPath Ścieżka materiału w scenie
         * @param newMaterial Flaga wskazująca czy materiał jest nowy
         */
        void AttachOrUpdateMaterial(
            const pxr::GfVec3f& diffuseColor,
            const float& IOR,
            const pxr::SdfPath& materialPath,
            bool newMaterial = false);


        /**
         * Metoda usuwająca powiązanie geometrii z obiektu sceny Embree
         * @param geometryID Indeks pod jakim geometria została powiązana w scenie
         */
        void DetachGeometryFromScene(uint geometryID);


        /**
         * Metoda szukająca powiązania ścieżki materiału z obiektem w buforze materiałów
         * @param materialPath Ścieżka szukanego materiału w scenie OpenUSD
         * @return Indeks materiału jeśli ścieżka została znaleziona. 0 jeśli materiał nie istnieje.
         * @remark Materiał pod indeksem 0 w buforze materiałów odpowiada domyślnemu materiałowi sceny.
         */
        uint GetIndexOfMaterialByPath(const pxr::SdfPath& materialPath) const;


        RTCDevice GetEmbreeDeviceHandle() const { return m_EmbreeDevice; };
        RTCScene  GetEmbreeSceneHandle() const  { return m_EmbreeScene; };

        void SetRenderArguments(const RenderArgument& renderArgument) { m_RenderArgument = renderArgument; }


    private:

        void WriteDataToSupportedAOV(const pxr::GfVec3f& colorOutput, const pxr::GfVec3f& normalOutput);

        /* EMBREE */

        /**
         * Metoda inicjalizująca obiekt geometrii bazowej światła o kształcie czworokąta.
         */
        void PrepareRectLightGeometrySource();

        /**
         * Geometria bazowa światła o kształcie czworokąta, stworzona z dwóch trójkątów.
         * Rozmiar geometrii oraz położenie wierzchołków są zgodne z założeniami OpenUSD
         * gdzie czworokąt jest zdefiniowany na płaszczyźnie XY, skierowany w -Z.
         *
         * Tworzenie świateł odbywa się na podstawie tworzenia instancji poniższej sceny / bazy / BVH.
         * Alternatywą jest tworzenie osobnej geometri oraz sceny dla każdego światła, jednak
         * jest to rozwiązanie mniej optymalne ze względu na użycie pamięci. Używanie instancingu
         * nie wiąże się z żadnym dodatkowym kosztem, wręcz jest zabiegiem optymalizacyjnym.
         */
        std::optional<RTCScene> m_RectLightPrimitiveScene;

        /**
         * Reprezentacja urządzenia w Embree (w przypadku macOS - CPU)
         * które jest odpowiedzialne za tworzenie obiektów biblioteki.
         */
        RTCDevice m_EmbreeDevice;


        /**
         * Wysoko-poziomowa reprezentacja struktury przyspieszenia intersekcji
         * (BVH - Bounding Volume Hierarchy) która jest drzewem brył ograniczających bibliteki Embree.
         */
        RTCScene m_EmbreeScene;

        /* MATERIAŁY */

        using PathMaterialPair = std::pair<pxr::SdfPath, std::unique_ptr<Material>>;

        /**
         * Bufor materiałów indeksowanych pośrednio za pomocą ścieżki materiału
         * w scenie. Użycie mapy nie jest wskazane ze względu na brak możliwości
         * szybkiego czytania za pomocą indeksu.
         * Przechowywanie ścieżki SdfPath jest wymagane do poprawnego powiązania
         * geometrii z materiałem.
         * Pierwszy materiał w buforze jest materiałem domyślnym używanym w przypadku braku powiązania.
         */
        std::vector<PathMaterialPair> m_MaterialBuffer;


        /* PROJEKCJA KAMERY */

        /**
         * Macierz projekcji stworzona na podstawie parametrów aktualnie wybranej kamery w scenie.
         */
        pxr::GfMatrix4d m_ProjectionMat;

        /**
         * Macierz transformacji kamery z object->world space.
         * Macierz view jest odwrotnością macierzy transformacji kamery w scenie.
         */
        pxr::GfMatrix4d m_ViewMat;

        /**
         * Macierz odwrotna projekcji
         */
        pxr::GfMatrix4d m_ProjectionMatInverse;

        /**
         * Macierz odwrotna widoku
         */
        pxr::GfMatrix4d m_ViewMatInverse;

        /**
         * Struktura przechowująca bufory wyjściowe oraz rozmiar wymaganego renderu.
         */
        RenderArgument m_RenderArgument;
    };

}