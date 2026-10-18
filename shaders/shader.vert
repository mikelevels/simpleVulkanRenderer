// shader.vert: the vertex shader, run once per vertex.
// GLSL (the OpenGL Shading Language), compiled to SPIR-V by glslc at build
// time (see CMakeLists.txt). The author's shader, with comments added.
#version 450

// No vertex buffer yet (Stage 16): the three corners are hard-coded and
// picked with gl_VertexIndex, the number of the vertex being processed
// (0, 1, 2 for vkCmdDraw(3, ...)).
//
// Vulkan's normalized device coordinates: x from -1 (left) to 1 (right),
// y from -1 (TOP) to 1 (BOTTOM). OpenGL and WebGPU have y pointing UP, so
// the same numbers draw upside down there.
vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),   // top middle
	vec2(0.5, 0.5),    // bottom right
	vec2(-0.5, 0.5)    // bottom left
);

vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),   // red
	vec3(0.0, 1.0, 0.0),   // green
	vec3(0.0, 0.0, 1.0)    // blue
);

// Passed to the fragment shader through "location 0", interpolated across
// the triangle: a pixel halfway between red and green gets yellow-ish.
layout(location = 0) out vec3 fragColor;

void main() {
	// (x, y, z, w). z = 0.0 is the near end of Vulkan's depth range [0, 1].
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}
