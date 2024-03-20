#pragma once

#include <memory>
#include <embree4/rtcore.h>


class OnyxRenderer
{
public:

    struct RenderArgument
    {
        unsigned int width;
        unsigned int height;

        size_t bufferElementSize;
        uint8_t* bufferData;
    };

    OnyxRenderer();
    ~OnyxRenderer();

    bool RenderColorAOV(const RenderArgument& renderArgument);

private:

    bool RenderEmbreeScene();

    RTCDevice m_EmbreeDevice;
    RTCScene m_EmbreeScene;
    RTCGeometry m_SceneGeometrySource;

};
