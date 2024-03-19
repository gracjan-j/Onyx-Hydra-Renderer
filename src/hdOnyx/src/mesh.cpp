#include "mesh.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdOnyxMesh::HdOnyxMesh(SdfPath const& id)
: HdMesh(id)
{

}


HdDirtyBits HdOnyxMesh::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::Clean | HdChangeTracker::DirtyTransform;
}


HdDirtyBits HdOnyxMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}


void HdOnyxMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{

}


void HdOnyxMesh::Sync(
    HdSceneDelegate *sceneDelegate
    , HdRenderParam *renderParam
    , HdDirtyBits *dirtyBits
    , TfToken const &reprToken)
{
    std::cout << "[hdOnyx] - Synchronizacja dla geometrii: " << GetId() << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
