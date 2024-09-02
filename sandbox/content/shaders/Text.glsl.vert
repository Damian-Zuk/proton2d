#version 450 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec2 TexCoord;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	Output.Color = Color;
	Output.TexCoord = TexCoord;

	gl_Position = u_ViewProjection * vec4(Position, 1.0);
}