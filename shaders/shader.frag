// shader.frag: the fragment shader, run once per pixel the triangle covers.
#version 450

// The vertex shader's output, blended between the three corners.
layout(location = 0) in vec3 fragColor;

// The color written to color attachment 0 (the swapchain image).
layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(fragColor, 1.0);   // alpha 1 = opaque
}
