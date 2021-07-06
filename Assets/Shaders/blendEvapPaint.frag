#version 430

layout(rgba32f, binding = 1) uniform image2D img_data;
layout(rgba32f, binding = 2) uniform image2D img_palette;

uniform	float dt;
uniform	float diffuseSpeed;
uniform	float evaporateSpeed;
uniform	int blendSize;
uniform float maxPixelValue;
uniform int paletteWidth;

out vec4 pixelColor;

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

void main() {

	vec2 id = gl_FragCoord.xy;

	// Blend 2*blendSize ^2 pixels
	float old = imageLoad(img_data, ivec2(id.xy) ).x;

	float accumulated = 0.0;
	for(int i = -blendSize; i <= blendSize; i++)
	{
		for(int j = -blendSize; j <= blendSize; j++)
		{
			accumulated += imageLoad(img_data, ivec2(id.xy) + ivec2(i, j) ).x;
		}
	}
	float factor = blendSize * 2.0 + 1;
	float average = accumulated / (factor * factor);
	float diffused = mix(old, average, diffuseSpeed * dt);

	// Evaporate
	float diffusedEvaporated = diffused - evaporateSpeed * dt;
	imageStore(img_data, ivec2(id), vec4(diffusedEvaporated, 0.0, 0.0, 0.0));

	// Lookup in palette
	float final = map(diffusedEvaporated, 0.0, maxPixelValue, 0.0, paletteWidth - 1);
	pixelColor = imageLoad(img_palette, ivec2(clamp(final, 0, paletteWidth), 0));
}
