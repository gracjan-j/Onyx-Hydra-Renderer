#pragma once

#include <embree4/rtcore_geometry.h>
#include <embree4/rtcore_ray.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/sdf/path.h>
#include <random>


#include "Integrator.h"
#include "RenderArgument.h"

namespace Onyx
{
    class Material;

    struct RayPayload
    {
        RTCRayHit RayHit;
        bool Terminated = false;
        uint8_t Bounce = 0;

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
        OnyxPathtracingIntegrator() = default;
        OnyxPathtracingIntegrator(const DataPayload& payload);

        ~OnyxPathtracingIntegrator() override;

        void Initialise(const DataPayload& payload);

        void PerformIteration() override;

        bool IsConverged() override;

        void ResetState() override;

        void SetRenderArgument(const std::shared_ptr<RenderArgument>& renderArgument);

    private:

        void PerformRayBounceIteration();
        bool IsRayBufferConverged();

        pxr::GfVec2f GenerateUniformRandomNumber2D();

        void ResetRayPayloadsWithPrimaryRays();
        void ResetSampleBuffer();

        std::shared_ptr<RenderArgument> m_RenderArgument;

        std::vector<RayPayload> m_RayPayloadBuffer;

        std::optional<pxr::GfVec2i> m_IntegrationResolution;
        std::optional<DataPayload> m_Data;

        std::random_device m_RandomDevice;
        std::mt19937 m_MersenneTwister;
        std::uniform_real_distribution<float> m_UniformDistributionGenerator;

        const uint8_t m_BounceLimit = 1;

        uint m_SampleCount = 1;
        uint m_SampleLimit = 1000;

        bool m_IncreaseSampleCount = true;
        std::vector<pxr::GfVec3f> m_SampleBuffer;
    };

}