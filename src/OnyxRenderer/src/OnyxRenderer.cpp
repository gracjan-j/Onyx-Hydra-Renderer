#include "OnyxRenderer.h"

#include <iostream>

OnyxRenderer::OnyxRenderer()
{
    m_EmbreeDevice = rtcNewDevice(NULL);
    m_EmbreeScene = rtcNewScene(m_EmbreeDevice);
    m_SceneGeometrySource = rtcNewGeometry(m_EmbreeDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
}

OnyxRenderer::~OnyxRenderer()
{
    rtcReleaseScene(m_EmbreeScene);
    rtcReleaseDevice(m_EmbreeDevice);
}

bool OnyxRenderer::RenderEmbreeScene()
{
    auto* vb = (float*) rtcSetNewGeometryBuffer(m_SceneGeometrySource,
      RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3*sizeof(float), 3);
    vb[0] = 0.f; vb[1] = 0.f; vb[2] = 0.f; // 1st vertex
    vb[3] = 1.f; vb[4] = 0.f; vb[5] = 0.f; // 2nd vertex
    vb[6] = 0.f; vb[7] = 1.f; vb[8] = 0.f; // 3rd vertex

    unsigned* ib = (unsigned*) rtcSetNewGeometryBuffer(m_SceneGeometrySource,
        RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3*sizeof(unsigned), 1);
    ib[0] = 0; ib[1] = 1; ib[2] = 2;

    rtcCommitGeometry(m_SceneGeometrySource);
    rtcAttachGeometry(m_EmbreeScene, m_SceneGeometrySource);
    rtcReleaseGeometry(m_SceneGeometrySource);
    rtcCommitScene(m_EmbreeScene);

    RTCRayHit rayhit;
    rayhit.ray.org_x  = 0.f; rayhit.ray.org_y = 0.f; rayhit.ray.org_z = -1.f;
    rayhit.ray.dir_x  = 0.f; rayhit.ray.dir_y = 0.f; rayhit.ray.dir_z =  1.f;
    rayhit.ray.tnear  = 0.f;
    rayhit.ray.tfar   = std::numeric_limits<float>::infinity();
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(m_EmbreeScene, &rayhit, nullptr);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        std::cout << "Intersection at t = " << rayhit.ray.tfar << std::endl;
    } else {
        std::cout << "No Intersection" << std::endl;
    }

    return true;
}


bool OnyxRenderer::RenderColorAOV(const RenderArgument& renderArgument)
{
    // RenderEmbreeScene();
    // return true;

    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < renderArgument.height; currentY++)
    {
        for (auto currentX = 0; currentX < renderArgument.width; currentX++)
        {
            // Obliczamy jedno-wymiarowy offset pixela w buforze 2D.
            uint32_t pixelOffsetInBuffer = (currentY * renderArgument.width) + currentX;

            uint8_t* bufferContents = renderArgument.bufferData;
            uint8_t* pixelData = &bufferContents[pixelOffsetInBuffer * renderArgument.bufferElementSize];

            // Wpisujemy wartość piksela na podstawie pozycji punktu
            // tworząc gradient.
            pixelData[0] = int((float(currentX) / renderArgument.width) * 255);
            pixelData[1] = int((float(currentY) / renderArgument.height) * 255);
            pixelData[2] = 255;
            pixelData[3] = 255;
        }
    }

    return true;
}