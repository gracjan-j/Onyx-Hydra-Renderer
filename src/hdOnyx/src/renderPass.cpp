#include "renderPass.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdOnyxRenderPass::HdOnyxRenderPass(HdRenderIndex *index, HdRprimCollection const &collection)
: HdRenderPass(index, collection)
{

}


HdOnyxRenderPass::~HdOnyxRenderPass()
{
    std::cout << "[hdOnyx] Destrukcja Render Pass" << std::endl;
}


void HdOnyxRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    std::cout << "[hdOnyx] => Wykonanie RenderPass" << std::endl;
}


PXR_NAMESPACE_CLOSE_SCOPE
