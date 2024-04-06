#pragma once

#include <memory>
#include <embree4/rtcore.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/usd/sdf/path.h>

#include "Material.h"
#include "RenderArgument.h"

namespace Onyx
{

    class OnyxRenderer
    {
    public:

        OnyxRenderer();
        ~OnyxRenderer();

        void MainRenderingEntrypoint(pxr::HdRenderThread* renderThread);

        bool RenderAllAOV();

        void SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix);
        void SetRenderArguments(const RenderArgument& renderArgument) { m_RenderArgument = renderArgument; }

        uint AttachGeometryToScene(const RTCGeometry& geometrySource);

        void AttachOrUpdateMaterial(
            const pxr::GfVec3f& diffuseColor,
            const float& IOR,
            const pxr::SdfPath& materialPath,
            bool newMaterial = false);

        void DetachGeometryFromScene(uint geometryID);

        uint GetIndexOfMaterialByPath(const pxr::SdfPath& materialPath);

        RTCDevice GetEmbreeDeviceHandle() const { return m_EmbreeDevice; };
        RTCScene GetEmbreeSceneHandle() const { return m_EmbreeScene; };

    private:

        void WriteDataToSupportedAOV(const pxr::GfVec3f& colorOutput, const pxr::GfVec3f& normalOutput);

        /* EMBREE */


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

        // Macierze perspektywy oraz widoku kamery wraz z inwersjami.
        // TODO: Dedykowana klasa Kamery, przygotowywanie RTCRay do Embree.
        pxr::GfMatrix4d m_ProjectionMat;
        pxr::GfMatrix4d m_ViewMat;
        pxr::GfMatrix4d m_ProjectionMatInverse;
        pxr::GfMatrix4d m_ViewMatInverse;

        RenderArgument m_RenderArgument;
    };

}