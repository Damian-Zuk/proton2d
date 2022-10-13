// Fragment Shader
#version 450 core

layout(location = 0) out vec4 o_Color;

struct VertexOutput
{
	vec4 Color;
	vec2 TextureCoords;
	float TilingFactor;
};

layout (location = 0) in VertexOutput Input;
layout (location = 3) in flat float v_TextureIndex;

layout (binding = 0) uniform sampler2D u_Textures[32];

void main()
{
	vec4 textureColor = Input.Color;

	textureColor *= texture(u_Textures[int(v_TextureIndex)], Input.TextureCoords * Input.TilingFactor);

	if (textureColor.a == 0.0)
		discard;

	o_Color = textureColor;
}