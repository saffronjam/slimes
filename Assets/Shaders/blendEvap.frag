#version 430

layout(rgba32f, binding = 1) uniform image2D img_data;

uniform	float dt;
uniform	float diffuseSpeed;
uniform	float evaporateSpeed;
uniform	int blendSize;

out vec4 pixelColor;

void main() {

	vec2 id = gl_FragCoord.xy;

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
	float diffusedEvaporated = diffused - evaporateSpeed * dt;

	pixelColor = vec4(diffusedEvaporated, 0.0, 0.0, 0.0);
}
