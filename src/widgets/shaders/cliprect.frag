#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
	mat4 qt_Matrix;
	float qt_Opacity;
	vec4 backgroundColor;
	vec4 borderColor;
};

layout(binding = 1) uniform sampler2D rect;
layout(binding = 2) uniform sampler2D content;

vec4 overlay(vec4 base, vec4 overlay) {
	if (overlay.a == 0.0) return base;
	if (base.a == 0.0) return overlay;

	vec3 rgb = overlay.rgb + base.rgb * (1.0 - overlay.a);
	float a = overlay.a + base.a * (1.0 - overlay.a);

	return vec4(rgb, a);
}

void main() {
	vec4 contentColor = texture(content, qt_TexCoord0.xy);
	vec4 rectColor = texture(rect, qt_TexCoord0.xy);

#ifdef CONTENT_UNDER_BORDER
	float contentAlpha = rectColor.a;
#else
	float contentAlpha = rectColor.r;
#endif

	float borderAlpha = rectColor.g;

	vec4 innerColor = overlay(backgroundColor, contentColor) * contentAlpha;
	vec4 borderColor = borderColor * borderAlpha;

	fragColor = (innerColor * (1.0 - borderColor.a) + borderColor) * qt_Opacity;
}
