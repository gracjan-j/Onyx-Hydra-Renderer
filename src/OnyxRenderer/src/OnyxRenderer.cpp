#include "OnyxRenderer.h"

#include <iostream>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/imaging/hd/tokens.h>

#include "OnyxHelper.h"
#include "../../hdOnyx/include/mesh.h"

OnyxRenderer::OnyxRenderer()
{
    m_EmbreeDevice = rtcNewDevice(NULL);
    m_EmbreeScene = rtcNewScene(m_EmbreeDevice);

    m_ProjectionMat.SetIdentity();
    m_ViewMat.SetIdentity();

    m_ProjectionMatInverse.SetIdentity();
    m_ViewMatInverse.SetIdentity();
}

uint OnyxRenderer::AttachGeometryToScene(const RTCGeometry& geometrySource)
{
    auto meshID = rtcAttachGeometry(m_EmbreeScene, geometrySource);

    if (meshID == RTC_INVALID_GEOMETRY_ID) {
        std::cout << "Mesh attachment error for: " << meshID << std::endl;
    }

    return meshID;
}


void OnyxRenderer::DetachGeometryFromScene(uint geometryID)
{
    // Odpinamy geometrię od sceny.
    rtcDetachGeometry(m_EmbreeScene, geometryID);
}



OnyxRenderer::~OnyxRenderer()
{
    rtcReleaseScene(m_EmbreeScene);
    rtcReleaseDevice(m_EmbreeDevice);
}


void OnyxRenderer::SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix)
{
    m_ProjectionMat = projMatrix;
    m_ViewMat = viewMatrix;

    m_ProjectionMatInverse = m_ProjectionMat.GetInverse();
    m_ViewMatInverse = m_ViewMat.GetInverse();
}


void writeNormalDataAOV(uint8_t* pixelDataStart, pxr::GfVec3f normal)
{
    pixelDataStart[0] = uint(((normal.data()[0] + 1.0) / 2.0) * 255);
    pixelDataStart[1] = uint(((normal.data()[1] + 1.0) / 2.0) * 255);
    pixelDataStart[2] = uint(((normal.data()[2] + 1.0) / 2.0) * 255);
    pixelDataStart[3] = 255;
}

pxr::GfVec3f TurboColorMap(float x) {
    float r = 0.1357 + x * ( 4.5974 - x * ( 42.3277 - x * ( 130.5887 - x * ( 150.5666 - x * 58.1375 ))));
    float g = 0.0914 + x * ( 2.1856 + x * ( 4.8052 - x * ( 14.0195 - x * ( 4.2109 + x * 2.7747 ))));
    float b = 0.1067 + x * ( 12.5925 - x * ( 60.1097 - x * ( 109.0745 - x * ( 88.5066 - x * 26.8183 ))));
    return {r, g, b};
}


bool OnyxRenderer::RenderAllAOV()
{
    // Zatwierdzamy scenę w obecnej postaci przed wywołaniem testu intersekcji.
    // Modyfikacje sceny nie są możliwe.
    rtcCommitScene(m_EmbreeScene);

    // Wyciągamy bufory danych do których silnik będzie wpisywał rezultat renderowania różnych zmiennych.
    auto colorAovBufferData = m_RenderArgument.GetBufferData(pxr::HdAovTokens->color);
    auto normalAovBufferData = m_RenderArgument.GetBufferData(pxr::HdAovTokens->normal);

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

    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < m_RenderArgument.height; currentY++)
    {
        for (auto currentX = 0; currentX < m_RenderArgument.width; currentX++)
        {
            // Obliczamy jedno-wymiarowy offset pixela w buforze 2D.
            uint32_t pixelOffsetInBuffer = (currentY * m_RenderArgument.width) + currentX;

            // Znajdujemy początek danych piksela w buforach AOV
            uint8_t* pixelDataNormal = writeNormalAOV
                ? &normalAovBuffer[pixelOffsetInBuffer * normalElementSize]
                : nullptr;

            uint8_t* pixelDataColor = writeColorAOV
                ? &colorAovBuffer[pixelOffsetInBuffer * colorElementSize]
                : nullptr;

            auto pixelColor = pxr::GfVec3f(0.0);
            auto pixelNormal = pxr::GfVec3f(0.0);

            // Generujemy promień z kamery.
            RTCRayHit primaryRayHit = OnyxHelper::GeneratePrimaryRay(
                currentX, currentY,
                m_RenderArgument.width, m_RenderArgument.height,
                m_ProjectionMatInverse, m_ViewMatInverse
            );

            // Dokonujemy testu intersekcji promienia ze sceną
            rtcIntersect1(m_EmbreeScene, &primaryRayHit, nullptr);

            // Jeżeli promień nie trafił w geometrię.
            if (primaryRayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
            {
                // Kończymy obliczenia dla piksela.
                if (writeColorAOV)
                {
                    uint8_t* colorPixelBufferData = &colorAovBuffer[pixelOffsetInBuffer * colorElementSize];
                    colorPixelBufferData[0] = 0; colorPixelBufferData[1] = 0;
                    colorPixelBufferData[2] = 0; colorPixelBufferData[3] = 255;
                }

                if (writeNormalAOV)
                {
                    writeNormalDataAOV(pixelDataNormal, pxr::GfVec3f(0.0));
                }

                // Przechodzimy do następnego piksela.
                continue;
            }

            pxr::GfVec3f hitWorldNormal = OnyxHelper::EvaluateHitSurfaceNormal(primaryRayHit, m_EmbreeScene);

            // Wpisujemy dane do normal AOV jeśli jest podpięte do silnika.
            if(writeNormalAOV)
            {
                writeNormalDataAOV(pixelDataNormal, hitWorldNormal);
            }

            if (writeColorAOV)
            {
                uint instanceID = primaryRayHit.hit.instID[0] * 1289;
                auto r = (instanceID % 256u);
                auto g = ((instanceID / 256) % 256);
                auto b = ((instanceID / (256u * 256u)) % 256u);

                pixelDataColor[0] = r;
                pixelDataColor[1] = g;
                pixelDataColor[2] = b;
                pixelDataColor[3] = 255;
            }
        }
    }

    return true;
}


void OnyxRenderer::MainRenderingEntrypoint(pxr::HdRenderThread* renderThread)
{
    while (renderThread->IsPauseRequested())
    {
        if (renderThread->IsStopRequested())
        {
            std::cout << "[Onyx Render Thread] Zatrzymano wątek renderowania po pauzie." << std::endl;
            return;
        }

        std::cout << "[Onyx] Pauza wątku renderowania." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (renderThread->IsStopRequested()) {
        std::cout << "[Onyx] Zatrzymano wątek renderowania." << std::endl;
        return;
    }

    RenderAllAOV();
}