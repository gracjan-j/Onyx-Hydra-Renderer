#pragma once

#include <memory>


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

    OnyxRenderer() {};
    ~OnyxRenderer() {};

    bool RenderColorAOV(const RenderArgument& renderArgument);

private:

};
