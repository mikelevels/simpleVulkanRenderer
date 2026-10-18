/// @file dynamic_state.cpp
/// @brief Every piece of drawing state, set one command at a time.
#include "dynamic_state.h"

namespace {

/// Where to draw, and how triangles become pixels.
void set_rasterization_state(vk::CommandBuffer cmd, vk::Extent2D frameSize) {
	// Viewport(x, y, width, height, minDepth, maxDepth): maps the -1..1
	// coordinates of the vertex shader to pixels.
	const vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(frameSize.width),
		static_cast<float>(frameSize.height), 0.0f, 1.0f);
	cmd.setViewportWithCount(viewport);
	cmd.setScissorWithCount(vk::Rect2D({ 0, 0 }, frameSize));   // no clipping
	cmd.setRasterizerDiscardEnable(VK_FALSE);   // do produce pixels
	cmd.setPolygonModeEXT(vk::PolygonMode::eFill);
	cmd.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);   // no MSAA
	const vk::SampleMask sampleMask = 0x1;
	cmd.setSampleMaskEXT(vk::SampleCountFlagBits::e1, sampleMask);
	cmd.setAlphaToCoverageEnableEXT(VK_FALSE);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);   // draw both sides
	// Which winding counts as "front". Unused while culling is off, but the
	// spec requires it whenever shader objects draw with rasterization on.
	// The author never sets it. Our SDK's validation layer (1.3.283) contains
	// this rule but doesn't catch it here: tested, no message. The layer is a
	// great net, not a proof (bugsExplained.md, entry 22).
	cmd.setFrontFace(vk::FrontFace::eClockwise);
	cmd.setDepthBiasEnable(VK_FALSE);
	cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
	cmd.setPrimitiveRestartEnable(VK_FALSE);
}

/// Depth/stencil tests, vertex input, and how colors are written.
void set_output_state(vk::CommandBuffer cmd) {
	cmd.setDepthTestEnable(VK_FALSE);
	cmd.setDepthWriteEnable(VK_FALSE);
	cmd.setStencilTestEnable(VK_FALSE);
	// No vertex buffers yet (Stage 16): an EMPTY vertex input description.
	// Also required by the spec when a vertex shader object is bound, and
	// missing in the author's Stage 10 (he adds it in Stage 14; entry 22).
	cmd.setVertexInputEXT(nullptr, nullptr);
	const vk::Bool32 blendEnable = VK_FALSE;   // overwrite, don't blend
	cmd.setColorBlendEnableEXT(0, blendEnable);
	// Only used when blending is on; harmless to set.
	const vk::ColorBlendEquationEXT equation(vk::BlendFactor::eOne, vk::BlendFactor::eZero,
		vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd);
	cmd.setColorBlendEquationEXT(0, equation);
	using C = vk::ColorComponentFlagBits;
	const vk::ColorComponentFlags writeMask = C::eR | C::eG | C::eB | C::eA;
	cmd.setColorWriteMaskEXT(0, writeMask);
}

}

void set_default_dynamic_state(vk::CommandBuffer commandBuffer, vk::Extent2D frameSize) {
	set_rasterization_state(commandBuffer, frameSize);
	set_output_state(commandBuffer);
}
