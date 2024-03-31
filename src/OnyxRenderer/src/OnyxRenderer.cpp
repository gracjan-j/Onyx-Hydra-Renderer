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


RTCRayHit OnyxRenderer::GeneratePrimaryRay(float offsetX, float offsetY, const RenderArgument& renderArgument)
{
    // Obliczamy Normalised Device Coordinates dla aktualnego piksela
    // NDC reprezentują pozycję pikela w screen-space.
    pxr::GfVec3f NDC {
        2.0f * (float(offsetX) / renderArgument.width) - 1.0f,
        2.0f * (float(offsetY) / renderArgument.height) - 1.0f,
        -1
    };

    // Przechodzimy z NDC (screen-space) do clip-space za pomocą odwrotnej projekcji
    // perspektywy wirtualnej kamery. -1 w NDC będzie odpowiadało bliskiej płaszczyźnie projekcji.
    const pxr::GfVec3f nearPlaneProjection = m_ProjectionMatInverse.Transform(NDC);

    pxr::GfVec3f origin = m_ViewMatInverse.Transform(pxr::GfVec3f(0.0, 0.0, 0.0));
    pxr::GfVec3f dir = m_ViewMatInverse.TransformDir(nearPlaneProjection).GetNormalized();

    RTCRayHit rayhit;
    rayhit.ray.org_x  = origin[0]; rayhit.ray.org_y = origin[1]; rayhit.ray.org_z = origin[2];;
    rayhit.ray.dir_x  = dir[0]; rayhit.ray.dir_y = dir[1]; rayhit.ray.dir_z = dir[2];
    rayhit.ray.tnear  = 0.0;
    rayhit.ray.tfar   = std::numeric_limits<float>::infinity();
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.ray.mask   = UINT_MAX;
    rayhit.ray.time   = 0.0;

    return rayhit;
}


bool OnyxRenderer::RenderColorAOV(const RenderArgument& renderArgument)
{
    // Zatwierdzamy scenę w obecnej postaci przed wywołaniem testu intersekcji.
    // Modyfikacje sceny nie są możliwe.
    rtcCommitScene(m_EmbreeScene);


    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < renderArgument.height; currentY++)
    {
        for (auto currentX = 0; currentX < renderArgument.width; currentX++)
        {
            // Obliczamy jedno-wymiarowy offset pixela w buforze 2D.
            uint32_t pixelOffsetInBuffer = (currentY * renderArgument.width) + currentX;

            uint8_t* bufferContents = renderArgument.bufferData;
            uint8_t* pixelData = &bufferContents[pixelOffsetInBuffer * renderArgument.bufferElementSize];

            RTCRayHit primaryRayHit = GeneratePrimaryRay(currentX, currentY, renderArgument);

            float intersectionResult = RenderEmbreeScene(primaryRayHit);

            if (intersectionResult > 0.0)
            {
                pixelData[2] = int(255 * std::clamp(float(intersectionResult / 10.0), 0.0f, 1.0f));
            }
        }
    }

    return true;
}


bool OnyxRenderer::RenderDebugNormalAOV(const RenderArgument& renderArgument)
{
    // Zatwierdzamy scenę w obecnej postaci przed wywołaniem testu intersekcji.
    // Modyfikacje sceny nie są możliwe.
    rtcCommitScene(m_EmbreeScene);


    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < renderArgument.height; currentY++)
    {
        for (auto currentX = 0; currentX < renderArgument.width; currentX++)
        {
            // Obliczamy jedno-wymiarowy offset pixela w buforze 2D.
            uint32_t pixelOffsetInBuffer = (currentY * renderArgument.width) + currentX;

            uint8_t* bufferContents = renderArgument.bufferData;
            uint8_t* pixelData = &bufferContents[pixelOffsetInBuffer * renderArgument.bufferElementSize];

            // Generujemy promień z kamery.
            RTCRayHit primaryRayHit = GeneratePrimaryRay(currentX, currentY, renderArgument);

            // Dokonujemy testu intersekcji promienia ze sceną
            rtcIntersect1(m_EmbreeScene, &primaryRayHit, nullptr);

            // Jeżeli promień nie trafił w geometrię, zwracamy czarne tło.
            if (primaryRayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
            {
                pixelData[0] = 0; pixelData[1] = 0; pixelData[2] = 0; pixelData[3] = 255;
                return true;
            }

            pxr::GfVec3f geometricNormal = { primaryRayHit.hit.Ng_x, primaryRayHit.hit.Ng_y, primaryRayHit.hit.Ng_z };
            geometricNormal.Normalize();
            geometricNormal *= 2.0;
            geometricNormal -= pxr::GfVec3f{1.0, 1.0, 1.0};

            pixelData[0] = uint(geometricNormal.data()[0] * 255);
            pixelData[1] = uint(geometricNormal.data()[1] * 255);
            pixelData[2] = uint(geometricNormal.data()[2] * 255);
            pixelData[3] = 255;
        }
    }

    return true;
}