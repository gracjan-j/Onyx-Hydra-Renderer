#include "../include/OnyxPathtracingIntegrator.h"

#include <embree4/rtcore.h>

#include "OnyxHelper.h"

#include "Material.h"

using namespace Onyx;


OnyxPathtracingIntegrator::~OnyxPathtracingIntegrator()
{
    m_RayPayloadBuffer.clear();
}

void OnyxPathtracingIntegrator::Initialise(const DataPayload& payload)
{
    m_Data = payload;
}


bool OnyxPathtracingIntegrator::IsConverged()
{
    if(m_RayPayloadBuffer.empty()) return false;

    for(auto& rayPayload : m_RayPayloadBuffer)
    {
        if(!rayPayload.Terminated) return false;
    }

    return true;
}


void OnyxPathtracingIntegrator::ResetState()
{
    ResetRayPayloadsWithPrimaryRays();
}


void OnyxPathtracingIntegrator::SetRenderArgument(
    const std::shared_ptr<RenderArgument>& renderArgument)
{
    m_RenderArgument = renderArgument;

    ResetState();
}


void OnyxPathtracingIntegrator::ResetRayPayloadsWithPrimaryRays()
{
    uint requiredBufferSize = m_RenderArgument->Width * m_RenderArgument->Height;

    // Resize dostosuje wielkość bufora. Nie ulegnie zmianie jeśli wymagany rozmiar == aktualny rozmiar.
    m_RayPayloadBuffer.resize(requiredBufferSize);

    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < m_RenderArgument->Height; currentY++)
    {
        for (auto currentX = 0; currentX < m_RenderArgument->Width; currentX++)
        {
            // Obliczamy jedno-wymiarowy offset promienia w buforze.
            uint32_t rayOffsetInBuffer = (currentY * m_RenderArgument->Width) + currentX;

            // Generujemy promień z kamery.
            RTCRayHit primaryRayHit = OnyxHelper::GeneratePrimaryRay(
                currentX, currentY,
                m_RenderArgument->Width, m_RenderArgument->Height,
                m_RenderArgument->MatrixInverseProjection, m_RenderArgument->MatrixInverseView
            );

            m_RayPayloadBuffer[rayOffsetInBuffer].RayHit = primaryRayHit;
            m_RayPayloadBuffer[rayOffsetInBuffer].Terminated = false;
            m_RayPayloadBuffer[rayOffsetInBuffer].Throughput = pxr::GfVec3f{1.0};
            m_RayPayloadBuffer[rayOffsetInBuffer].Radiance = pxr::GfVec3f{0.0};
        }
    }
}


void writeNormalDataAOV(uint8_t* pixelDataStart, pxr::GfVec3f normal)
{
    pixelDataStart[0] = uint(((normal[0] + 1.0) / 2.0) * 255);
    pixelDataStart[1] = uint(((normal[1] + 1.0) / 2.0) * 255);
    pixelDataStart[2] = uint(((normal[2] + 1.0) / 2.0) * 255);
    pixelDataStart[3] = 255;
}

void writeColorDataAOV(uint8_t* pixelDataStart, pxr::GfVec3f color)
{
    auto floatToRGB = pxr::GfCompMult(color, pxr::GfVec3f(255.0));
    pixelDataStart[0] = std::clamp(uint(floatToRGB[0]), 0u, 255u);
    pixelDataStart[1] = std::clamp(uint(floatToRGB[1]), 0u, 255u);
    pixelDataStart[2] = std::clamp(uint(floatToRGB[2]), 0u, 255u);
    pixelDataStart[3] = 255u;
}


void OnyxPathtracingIntegrator::PerformIteration()
{
    if(!m_IntegrationResolution.has_value()
        || m_RenderArgument->SizeChanged(
            m_IntegrationResolution.value()[0],
            m_IntegrationResolution.value()[1]))
    {
        ResetState();
    }

    // W przypadku braku AOV koloru wykonywnanie pełnego algorytmu nie jest konieczne.
    // Sprawdzamy dostępność tego kanału danych.
    uint aovIndex;
    bool hasColorAOV = m_RenderArgument->IsAvailable(pxr::HdAovTokens->color, aovIndex);

    // Wyciągamy bufory danych do których silnik będzie wpisywał rezultat renderowania różnych zmiennych.
    auto colorAovBufferData = m_RenderArgument->GetBufferData(pxr::HdAovTokens->color);
    auto normalAovBufferData = m_RenderArgument->GetBufferData(pxr::HdAovTokens->normal);

    bool writeColorAOV = colorAovBufferData.has_value();
    bool writeNormalAOV = normalAovBufferData.has_value();

    uint8_t* colorAovBuffer;
    uint8_t* normalAovBuffer;
    size_t colorElementSize, normalElementSize;
    if (writeColorAOV)
    {
        colorAovBuffer = static_cast<uint8_t*>(colorAovBufferData.value().first);
        colorElementSize = colorAovBufferData.value().second;
    }

    if (writeNormalAOV)
    {
        normalAovBuffer = static_cast<uint8_t*>(normalAovBufferData.value().first);
        normalElementSize = normalAovBufferData.value().second;
    }

    for(int rayIndex = 0; rayIndex < m_RayPayloadBuffer.size(); rayIndex++)
    {
        // Znajdujemy początek danych piksela w buforach AOV
        uint8_t* pixelDataNormal = writeNormalAOV
            ? &normalAovBuffer[rayIndex * normalElementSize]
            : nullptr;

        uint8_t* pixelDataColor = writeColorAOV
            ? &colorAovBuffer[rayIndex * colorElementSize]
            : nullptr;

        auto& currentPayload = m_RayPayloadBuffer[rayIndex];

        // Dokonujemy testu intersekcji promienia ze sceną
        rtcIntersect1(*m_Data->Scene, &currentPayload.RayHit, nullptr);

        // Jeżeli promień nie trafił w geometrię.
        if (currentPayload.RayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            // Kończymy obliczenia dla piksela.
            if (writeColorAOV) writeColorDataAOV(pixelDataColor, pxr::GfVec3f(0.0));
            if (writeNormalAOV) writeNormalDataAOV(pixelDataNormal, pxr::GfVec3f(0.0));
            currentPayload.Terminated = true;
            // Przechodzimy do następnego piksela.
            continue;
        }

        // Pobieramy strukturę pomocniczą powiązaną z instancją
        auto* hitInstanceData = static_cast<pxr::HdOnyxInstanceData*>(
            rtcGetGeometryUserData(
                // Identyfikator instancji zakłada jedno-poziomowy instancing ([0]).
                // Zgodne z założeniem w HdOnyxMesh który tworzy geometrię.
                rtcGetGeometry(*m_Data->Scene, currentPayload.RayHit.hit.instID[0])
            )
        );

        // Jeśli promień uderzył w światło
        if (hitInstanceData->Light)
        {
            // Pobieramy dane instancji światła
            auto& lightEmission = m_Data->LightBuffer->at(hitInstanceData->DataIndexInBuffer);

            currentPayload.Radiance += GfCompMult(currentPayload.Throughput, lightEmission);
            currentPayload.Terminated = true;

            if (writeColorAOV) writeColorDataAOV(pixelDataColor, currentPayload.Radiance);

            // Przechodzimy do następnego piksela.
            continue;
        }

        // Obliczamy wektor normalny powierzchni.
        pxr::GfVec3f hitWorldNormal = OnyxHelper::EvaluateHitSurfaceNormal(currentPayload.RayHit, *m_Data->Scene);

        if(writeNormalAOV && !writeColorAOV)
        {
            writeNormalDataAOV(pixelDataNormal, hitWorldNormal);
            currentPayload.Terminated = true;
            continue;
        }



        if (writeColorAOV)
        {
            uint instanceID = currentPayload.RayHit.hit.instID[0] * 1289;
            auto r = (instanceID % 256u);
            auto g = ((instanceID / 256) % 256);
            auto b = ((instanceID / (256u * 256u)) % 256u);

            pixelDataColor[0] = r;
            pixelDataColor[1] = g;
            pixelDataColor[2] = b;
            pixelDataColor[3] = 255;
            auto dataID = hitInstanceData->DataIndexInBuffer;

            if (!hitInstanceData->Light)
            {
                auto& boundMaterial = m_Data->MaterialBuffer->at(dataID);
                auto diffuseColor = boundMaterial.second->Evaluate();
                pixelDataColor[0] = int(diffuseColor[0] * 255);
                pixelDataColor[1] = int(diffuseColor[1] * 255);
                pixelDataColor[2] = int(diffuseColor[2] * 255);
                pixelDataColor[3] = 255;
            }
            else
            {
                auto& lightData = m_Data->LightBuffer->at(dataID);
                pixelDataColor[0] = int(lightData[0] * 255);
                pixelDataColor[1] = int(lightData[1] * 255);
                pixelDataColor[2] = int(lightData[2] * 255);
                pixelDataColor[3] = 255;
            }
        }

        m_RayPayloadBuffer[rayIndex].Terminated = true;
    }
}
