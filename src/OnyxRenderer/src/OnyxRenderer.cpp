#include "OnyxRenderer.h"

#include <iostream>

OnyxRenderer::OnyxRenderer()
{
    m_EmbreeDevice = rtcNewDevice(NULL);
    m_EmbreeScene = rtcNewScene(m_EmbreeDevice);
    m_SceneGeometrySource = rtcNewGeometry(m_EmbreeDevice, RTC_GEOMETRY_TYPE_TRIANGLE);

    m_ProjectionMat.SetIdentity();
    m_ViewMat.SetIdentity();

    m_ProjectionMatInverse.SetIdentity();
    m_ViewMatInverse.SetIdentity();

    auto* vb = (float*) rtcSetNewGeometryBuffer(m_SceneGeometrySource,
      RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3*sizeof(float), 3);
    vb[0] = 0.0f; vb[1] = 0.0f; vb[2] = 0.0f; // 1st vertex
    vb[3] = 1.f; vb[4] = 0.f; vb[5] = 0.f; // 2nd vertex
    vb[6] = 0.f; vb[7] = 1.f; vb[8] = 0.f; // 3rd vertex

    unsigned* ib = (unsigned*) rtcSetNewGeometryBuffer(m_SceneGeometrySource,
        RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3*sizeof(unsigned), 1);
    ib[0] = 0; ib[1] = 1; ib[2] = 2;

    rtcCommitGeometry(m_SceneGeometrySource);
    rtcAttachGeometry(m_EmbreeScene, m_SceneGeometrySource);
    rtcReleaseGeometry(m_SceneGeometrySource);
    rtcCommitScene(m_EmbreeScene);
}

OnyxRenderer::~OnyxRenderer()
{
    rtcReleaseScene(m_EmbreeScene);
    rtcReleaseDevice(m_EmbreeDevice);
}

float OnyxRenderer::RenderEmbreeScene(RTCRayHit ray)
{
    rtcIntersect1(m_EmbreeScene, &ray, nullptr);

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID)
    {
        return 0.0;
    }

    return 1.0;
}

void OnyxRenderer::SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix)
{
    m_ProjectionMat = projMatrix;
    m_ViewMat = viewMatrix;

    m_ProjectionMatInverse = m_ProjectionMat.GetInverse();
    m_ViewMatInverse = m_ViewMat.GetInverse();
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
            pixelData[0] = int((float(currentX) / renderArgument.width) * 0);
            pixelData[1] = int((float(currentY) / renderArgument.height) * 0);
            pixelData[2] = 0;
            pixelData[3] = 255;

            // Obliczamy Normalised Device Coordinates dla aktualnego piksela
            // NDC reprezentują pozycję pikela w screen-space.
            pxr::GfVec3f NDC {
                2.0f * (float(currentX) / renderArgument.width) - 1.0f,
                2.0f * (float(currentY) / renderArgument.width) - 1.0f,
                -1
            };

            // Przechodzimy z NDC (screen-space) do clip-space za pomocą odwrotnej projekcji
            // perspektywy wirtualnej kamery. -1 w NDC będzie odpowiadało bliskiej płaszczyźnie projekcji.
            const pxr::GfVec3f nearPlaneProjection = m_ProjectionMatInverse.Transform(NDC);

            pxr::GfVec3f origin;
            pxr::GfVec3f dir;

            origin = pxr::GfVec3f(0,0,0);
            dir = nearPlaneProjection;

            // Transform camera rays to world space.
            origin = m_ViewMatInverse.Transform(origin);
            dir = m_ViewMatInverse.TransformDir(dir).GetNormalized();

            RTCRayHit rayhit;
            rayhit.ray.org_x  = origin[0]; rayhit.ray.org_y = origin[1]; rayhit.ray.org_z = origin[2];;
            rayhit.ray.dir_x  = dir[0]; rayhit.ray.dir_y = dir[1]; rayhit.ray.dir_z = dir[2];
            rayhit.ray.tnear  = 0.0;
            rayhit.ray.tfar   = std::numeric_limits<float>::infinity();
            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            float intersectionResult = RenderEmbreeScene(rayhit);

            pixelData[2] = int(254.0 * intersectionResult);
        }
    }

    return true;
}