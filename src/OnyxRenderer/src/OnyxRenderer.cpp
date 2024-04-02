#include "OnyxRenderer.h"

#include <iostream>

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

float OnyxRenderer::RenderEmbreeScene(RTCRayHit ray)
{
    rtcIntersect1(m_EmbreeScene, &ray, nullptr);

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID)
    {
        return 0.0;
    }

    return ray.ray.tfar;
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
    // Zatwierdzamy scenę w obecnej postaci przed wywołaniem testu intersekcji.
    // Modyfikacje sceny nie są możliwe.
    rtcCommitScene(m_EmbreeScene);

    auto err = rtcGetDeviceError(m_EmbreeDevice);

    if (err == RTC_ERROR_NONE)
    {
        int i = 0;
    }

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

            pxr::GfVec3f origin = m_ViewMatInverse.Transform(pxr::GfVec3f(0.0, 0.0, 0.0));
            pxr::GfVec3f dir = m_ViewMatInverse.TransformDir(nearPlaneProjection).GetNormalized();

            RTCBounds out;
            rtcGetSceneBounds(m_EmbreeScene, &out);

            RTCRayHit rayhit;
            rayhit.ray.org_x  = origin[0]; rayhit.ray.org_y = origin[1]; rayhit.ray.org_z = origin[2];;
            rayhit.ray.dir_x  = dir[0]; rayhit.ray.dir_y = dir[1]; rayhit.ray.dir_z = dir[2];
            rayhit.ray.tnear  = 0.0;
            rayhit.ray.tfar   = std::numeric_limits<float>::infinity();
            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rayhit.ray.mask   = UINT_MAX;
            rayhit.ray.time   = 0.0;

            float intersectionResult = RenderEmbreeScene(rayhit);

            if (intersectionResult > 0.0)
            {
                pixelData[2] = int(255 * intersectionResult / 10.0);
            }

        }
    }

    return true;
}