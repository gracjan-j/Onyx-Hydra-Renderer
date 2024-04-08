#include "../include/OnyxPathtracingIntegrator.h"

#include <embree4/rtcore.h>

#include "OnyxHelper.h"

#include "Material.h"

using namespace Onyx;


OnyxPathtracingIntegrator::OnyxPathtracingIntegrator(const DataPayload& payload)
: m_Data(payload)
, m_MersenneTwister(m_RandomDevice())
, m_UniformDistributionGenerator(std::uniform_real_distribution<float>(0.0f, 1.0f))
{
}


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


void OnyxPathtracingIntegrator::SetRenderArgument(const std::shared_ptr<RenderArgument>& renderArgument)
{
    m_RenderArgument = renderArgument;

    ResetState();
}


pxr::GfVec2f OnyxPathtracingIntegrator::GenerateRandomNumber2D()
{
    return {
        m_UniformDistributionGenerator(m_MersenneTwister),
        m_UniformDistributionGenerator(m_MersenneTwister)
    };
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
            m_RayPayloadBuffer[rayOffsetInBuffer].Bounce = 0;
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
        m_IntegrationResolution = pxr::GfVec2i(int(m_RenderArgument->Width), int(m_RenderArgument->Height));
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

        // Jeśli promień przekroczył limit ilości odbić bez znalezienia światła.
        if (currentPayload.Bounce > m_BounceLimit)
        {
            // Ręcznie zatrzymujemy promień
            if (writeColorAOV) writeColorDataAOV(pixelDataColor, pxr::GfVec3f(0.0));
            if (writeNormalAOV) writeNormalDataAOV(pixelDataNormal, pxr::GfVec3f(0.0));
            currentPayload.Terminated = true;
            // Przechodzimy do następnego promienia.
            continue;
        }

        // Dokonujemy testu intersekcji promienia ze sceną
        rtcIntersect1(*m_Data->Scene, &currentPayload.RayHit, nullptr);

        // Jeżeli promień nie trafił w geometrię.
        if (currentPayload.RayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            // Kończymy obliczenia dla piksela.
            if (writeColorAOV) writeColorDataAOV(pixelDataColor, pxr::GfVec3f(0.0));
            if (writeNormalAOV) writeNormalDataAOV(pixelDataNormal, pxr::GfVec3f(0.0));
            currentPayload.Terminated = true;
            // Przechodzimy do następnego promienia.
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

        if (rayIndex == 10)
        {
            int a;
        }

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

        // Promień nie uderzył w światło lecz geometrię.
        // Pobieramy materiał podpięty do geometrii aby wygenerować odbicie promienia na powierzchni.
        auto dataID = hitInstanceData->DataIndexInBuffer;
        auto& boundMaterial = m_Data->MaterialBuffer->at(dataID);

        // Generujemy odbicie na powierzchni materiału za pomocą dedykowanej metody.
        // Metoda generuje próbkę w local space na podstawie parametrów materiału.
        // Przekazanie wektora normalnego pozwala na transformację wygenerowanej próbki
        // do world-space w orientacji zgodnej z wektorem normalnym powierzchni.
        auto rand2D = pxr::GfVec2f{
            m_UniformDistributionGenerator(m_MersenneTwister),
            m_UniformDistributionGenerator(m_MersenneTwister)
        };
        auto materialSampleDir = boundMaterial.second->Sample(hitWorldNormal, rand2D);

        // Skalujemy siłę naszego promienia przez funkcję BXDF materiału
        // która skaluje ilość mocy wejściowej na podstawie charakterystyki materiału.
        currentPayload.Throughput = pxr::GfCompMult(
            boundMaterial.second->Evaluate(materialSampleDir) / boundMaterial.second->PDF(materialSampleDir),
            currentPayload.Throughput
        );

        // Obliczamy pozycję intersekcji w świecie.
        // Pozycja = kierunek * czas + początek
        auto origin = pxr::GfVec3f(
            currentPayload.RayHit.ray.org_x,
            currentPayload.RayHit.ray.org_y,
            currentPayload.RayHit.ray.org_z);
        auto direction = pxr::GfVec3f(
                    currentPayload.RayHit.ray.dir_x,
                    currentPayload.RayHit.ray.dir_y,
                    currentPayload.RayHit.ray.dir_z);

        auto hitPosition = direction * currentPayload.RayHit.ray.tfar + origin;

        // Generujemy promień odbicia który zaczyna się w punkcie ostatniej intersekcji z geometrią
        // o kierunku odbicia który został wygenerowany na podstawie funkcji BXDF materiału.
        // Dokonujemy śledzenia ścieżki do momentu zakończenia tego procesu przez trafienie w światło.
        auto bounceRay = OnyxHelper::GenerateBounceRay(materialSampleDir, hitPosition, hitWorldNormal);

        // Podmieniamy promień dla następnej iteracji.
        currentPayload.RayHit = bounceRay;

        // Odbicie oznacza kolejną iterację.
        currentPayload.Terminated = false;
        currentPayload.Bounce += 1;
    }
}
