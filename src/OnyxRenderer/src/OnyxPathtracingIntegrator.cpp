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
    // Jeżeli zebraliśmy wymaganą ilość próbek.
    if (m_SampleCount >= m_SampleLimit) return true;
    return false;
}


void OnyxPathtracingIntegrator::ResetState()
{
    ResetRayPayloadsWithPrimaryRays();
    ResetSampleBuffer();
}


void OnyxPathtracingIntegrator::SetRenderArgument(const std::shared_ptr<RenderArgument>& renderArgument)
{
    m_RenderArgument = renderArgument;

    ResetState();
}


pxr::GfVec2f OnyxPathtracingIntegrator::GenerateUniformRandomNumber2D()
{
    return {
        m_UniformDistributionGenerator(m_MersenneTwister),
        m_UniformDistributionGenerator(m_MersenneTwister)};
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

            // Generujemy dwie liczby losowe do wygenerowania promienia.
            auto uniform2D = pxr::GfVec2f{
                m_UniformDistributionGenerator(m_MersenneTwister),
                m_UniformDistributionGenerator(m_MersenneTwister)
            };

            // Generujemy promień z kamery.
            RTCRayHit primaryRayHit = OnyxHelper::GeneratePrimaryRay(
                currentX, currentY, m_RenderArgument->Width, m_RenderArgument->Height,
                m_RenderArgument->MatrixInverseProjection, m_RenderArgument->MatrixInverseView,
                uniform2D);

            m_RayPayloadBuffer[rayOffsetInBuffer].RayHit = primaryRayHit;
            m_RayPayloadBuffer[rayOffsetInBuffer].Terminated = false;
            m_RayPayloadBuffer[rayOffsetInBuffer].Bounce = 0;
            m_RayPayloadBuffer[rayOffsetInBuffer].Throughput = pxr::GfVec3f{1.0};
            m_RayPayloadBuffer[rayOffsetInBuffer].Radiance = pxr::GfVec3f{0.0};
        }
    }
}


void OnyxPathtracingIntegrator::ResetSampleBuffer()
{
    uint requiredBufferSize = m_RenderArgument->Width * m_RenderArgument->Height;

    // Resize dostosuje wielkość bufora. Nie ulegnie zmianie jeśli wymagany rozmiar == aktualny rozmiar.
    m_SampleBuffer.resize(requiredBufferSize);

    for (auto& sample: m_SampleBuffer)
    {
        sample.Set(0.0, 0.0, 0.0);
    }

    m_SampleCount = 1;
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

    if(m_SampleCount > m_SampleLimit) return;

    m_IncreaseSampleCount = true;

    // Wykonujemy śledzenie segmentu ścieżki do momentu zatrzymania każdego z promieni w buforze.
    while(!IsRayBufferConverged())
    {
        // Wykonujemy jedną iterację śledzenia (jedno odbicie promienia - jeden segment ścieżki)
        PerformRayBounceIteration();
    }

    // Jedna iteracja integratora = wykonanie śledzenia ścieżek (grupy segmentów) dla jednego piksela.
    // Wykonanie wielu iteracji integratora pozwala nam na poprawę jakości aproksymacji
    // zgodnie z teorią Monte Carlo. Wyniki zostaną uśrednione przez ilość zebranych próbek (Sample Count).
    if (m_IncreaseSampleCount) m_SampleCount += 1;

    // Wykonanie nowej iteracji ponownie zaczyna się w kamerze. Wypełniamy bufor promieni promieniem "primary"
    // (promień wychodzący z kamery).
    ResetRayPayloadsWithPrimaryRays();
}


void OnyxPathtracingIntegrator::PerformRayBounceIteration()
{
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

    for (int rayIndex = 0; rayIndex < m_RayPayloadBuffer.size(); rayIndex++)
    {
        // Znajdujemy początek danych piksela odpowiadającego promieniowi w buforze AOV
        uint8_t* pixelDataNormal = writeNormalAOV ? &normalAovBuffer[rayIndex * normalElementSize] : nullptr;
        uint8_t* pixelDataColor = writeColorAOV ? &colorAovBuffer[rayIndex * colorElementSize] : nullptr;

        auto& currentPayload = m_RayPayloadBuffer[rayIndex];

        // Jeśli działanie promienia zostało już wcześniej zakończone, pomijamy go.
        if (currentPayload.Terminated) continue;

        // Jeśli promień przekroczył limit ilości odbić bez znalezienia światła.
        if (currentPayload.Bounce > m_BounceLimit)
        {
            if (writeNormalAOV) writeNormalDataAOV(pixelDataNormal, pxr::GfVec3f(0.0));

            // Kończymy działanie promienia.
            currentPayload.Radiance.Set(0.0, 0.0, 0.0);
            currentPayload.Terminated = true;

            // Przechodzimy do następnego promienia.
            continue;
        }

        // Dokonujemy testu intersekcji promienia ze sceną.
        rtcIntersect1(*m_Data->Scene, &currentPayload.RayHit, nullptr);

        // Jeżeli promień nie trafił w geometrię.
        if (currentPayload.RayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
        {
            if (writeNormalAOV)
                writeNormalDataAOV(pixelDataNormal, pxr::GfVec3f(0.0));

            currentPayload.Terminated = true;
            // Przechodzimy do następnego promienia.
            continue;
        }

        // Pobieramy strukturę pomocniczą powiązaną z instancją
        auto* hitInstanceData = static_cast<pxr::HdOnyxInstanceData*>(rtcGetGeometryUserData(
            // Identyfikator instancji zakłada jedno-poziomowy instancing ([0])
            // zgodne z założeniem w HdOnyxMesh który tworzy geometrię.
            rtcGetGeometry(*m_Data->Scene, currentPayload.RayHit.hit.instID[0])));

        // Jeśli promień uderzył w światło
        if (hitInstanceData->Light)
        {
            // Pobieramy dane instancji światła
            auto& lightEmission = m_Data->LightBuffer->at(hitInstanceData->DataIndexInBuffer);

            // Dodajemy moc światła przeskalowaną przez ścieżki (moc ścieżki jest skalowana przez
            // refleksyjność powierzchni przy każdym odbiciu od geometrii).
            currentPayload.Radiance += GfCompMult(currentPayload.Throughput, lightEmission);

            m_SampleBuffer[rayIndex] += pxr::GfVec3f(currentPayload.Radiance);
            if (writeColorAOV) writeColorDataAOV(pixelDataColor, m_SampleBuffer[rayIndex] / m_SampleCount);

            // Kończymy działanie promienia.
            currentPayload.Terminated = true;

            // Przechodzimy do następnego piksela.
            continue;
        }

        // Obliczamy wektor normalny powierzchni.
        pxr::GfVec3f hitWorldNormal = OnyxHelper::EvaluateHitSurfaceNormal(currentPayload.RayHit, *m_Data->Scene);

        // Jeżeli wymagane jest jedynie zrwócenie wektora normalnego dla pierwszego uderzenia.
        if (writeNormalAOV && !writeColorAOV)
        {
            writeNormalDataAOV(pixelDataNormal, hitWorldNormal);
            currentPayload.Terminated = true;
            m_IncreaseSampleCount = false;
            continue;
        }

        // Promień nie uderzył w światło lecz geometrię.
        // Pobieramy materiał powiązany z geometrią aby wygenerować odbicie promienia na powierzchni.
        auto dataID = hitInstanceData->DataIndexInBuffer;
        auto& boundMaterial = m_Data->MaterialBuffer->at(dataID);

        // Generujemy odbicie na powierzchni materiału za pomocą dedykowanej metody.
        // Metoda generuje próbkę w local space na podstawie parametrów materiału.
        // Przekazanie wektora normalnego pozwala na transformację wygenerowanej próbki
        // do world-space w orientacji zgodnej z wektorem normalnym powierzchni.
        auto rand2D = pxr::GfVec2f{m_UniformDistributionGenerator(m_MersenneTwister),
                                   m_UniformDistributionGenerator(m_MersenneTwister)};
        auto materialSampleDir = boundMaterial.second->Sample(hitWorldNormal, rand2D);

        // Skalujemy siłę naszego promienia przez funkcję BXDF materiału.
        // Funkcja BXDR określa stosunek mocy wejściowej do mocy wyjściowej na podstawie charakterystyki materiału.
        currentPayload.Throughput = pxr::GfCompMult(
            boundMaterial.second->Evaluate(materialSampleDir) / boundMaterial.second->PDF(materialSampleDir),
            currentPayload.Throughput
        );

        // Obliczamy pozycję intersekcji w świecie.
        // Pozycja = kierunek * czas + początek
        auto origin = pxr::GfVec3f(currentPayload.RayHit.ray.org_x, currentPayload.RayHit.ray.org_y,
                                   currentPayload.RayHit.ray.org_z);
        auto direction = pxr::GfVec3f(currentPayload.RayHit.ray.dir_x, currentPayload.RayHit.ray.dir_y,
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


bool OnyxPathtracingIntegrator::IsRayBufferConverged()
{
    if(m_RayPayloadBuffer.empty()) return true;

    for(auto& rayPayload : m_RayPayloadBuffer)
    {
        if(!rayPayload.Terminated) return false;
    }

    return true;
}

