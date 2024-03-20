#pragma once

#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/base/gf/vec3i.h>


PXR_NAMESPACE_OPEN_SCOPE


class HdOnyxRenderBuffer : public HdRenderBuffer
{
public:
    HdOnyxRenderBuffer(const SdfPath& bprimId);

    virtual bool Allocate(const GfVec3i& dimensions, HdFormat format, bool multiSampled) override;


    virtual unsigned int GetWidth() const override;
    virtual unsigned int GetHeight() const override;
    virtual unsigned int GetDepth() const override;

    virtual HdFormat GetFormat() const override;

    virtual bool IsMultiSampled() const override;

    virtual void* Map() override;
    virtual void Unmap() override;
    virtual bool IsMapped() const override;

    virtual bool IsConverged() const override;

    virtual void Resolve() override;

private:
    virtual void _Deallocate() override;

    HdFormat m_DataFormat = HdFormatInvalid;
    GfVec3i m_Dimensions = GfVec3i(0, 0, 0);

    std::vector<uint8_t> m_DataVector;

    std::atomic<bool> m_Converged = { false };
    std::atomic<int> m_MappedUsersCount = { 0 };
};

PXR_NAMESPACE_CLOSE_SCOPE
