#version 450 core
layout(binding = 0) uniform sampler2D output_image;
in vec2 texCoord;
out vec4 fragColor;
void main() {
	fragColor = texture(output_image, texCoord);
}