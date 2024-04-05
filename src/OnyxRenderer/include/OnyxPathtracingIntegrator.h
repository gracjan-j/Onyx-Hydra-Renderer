#pragma once

#include "Integrator.h"

namespace Onyx
{

    class OnyxPathtracingIntegrator final : Integrator
    {
    public:
        ~OnyxPathtracingIntegrator() override;

        void PerformIteration() override;

        bool IsConverged() override;

        void ResetState() override;

    private:

    };

}