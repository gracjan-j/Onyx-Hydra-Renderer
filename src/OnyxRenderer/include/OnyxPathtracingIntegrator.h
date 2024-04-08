#pragma once

#include <embree4/rtcore_geometry.h>
#include <embree4/rtcore_ray.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/sdf/path.h>


#include "Integrator.h"
#include "RenderArgument.h"

namespace Onyx
{
    class Material;

    struct RayPayload
    {
        RTCRayHit RayHit;
        bool Terminated;

        pxr::GfVec3f Throughput = pxr::GfVec3f(1.0);
        pxr::GfVec3f Radiance = pxr::GfVec3f(0.0);
    };

    struct DataPayload
    {
        RTCScene* Scene;
        std::vector<pxr::GfVec3f>* LightBuffer;
        std::vector<std::pair<pxr::SdfPath, std::unique_ptr<Material>>>* MaterialBuffer;
    };

    class OnyxPathtracingIntegrator final : Integrator
    {
    public:
        OnyxPathtracingIntegrator(const DataPayload& payload)
        : m_Data(payload) {};

        ~OnyxPathtracingIntegrator() override;

        void Initialise(const DataPayload& payload);

        void PerformIteration() override;

        bool IsConverged() override;

        void ResetState() override;

        void SetRenderArgument(const std::shared_ptr<RenderArgument>& renderArgument);

    private:

        void ResetRayPayloadsWithPrimaryRays();

        std::shared_ptr<RenderArgument> m_RenderArgument;

        std::vector<RayPayload> m_RayPayloadBuffer;

        std::optional<pxr::GfVec2i> m_IntegrationResolution;

        std::optional<DataPayload> m_Data;
    };

}