#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D landscapeTexture;
uniform float iTime;
uniform vec3 iResolution; // x: width, y: height, z: aspect ratio

void main()
{             
    vec2 uv = TexCoords;

	vec4 textureData = texture(landscapeTexture, uv);

    float heightValue = textureData.b * 255;

	// Convert float (0-1) to uint8 (0-255) and clamp values
	if (textureData.g == 1.0) {
		// On Fire
		FragColor = vec4(vec3(255., 119., 0.) / 255., 1.0);
	}
	else if (textureData.g == 2.0f) {
		// Destroyed
		FragColor = vec4(vec3(0., 0., 0.) / 255., 1.0);
	}
	else if (textureData.r == 0.0f) {
		// Grass
		FragColor = vec4(vec3(min(255., 121. + heightValue), min(255., 150. + heightValue), min(255., 114. + heightValue)) / 255., 1.0);
	}
	else if (textureData.r == 1.0f) {
		// Water
		FragColor = vec4(vec3(181., 219., 235.) / 255., 1.0);
	}
	else if (textureData.r == 2.0f) {
		// Bedrock
		FragColor = vec4(vec3(min(255., 207. + heightValue), min(255., 198. + heightValue), min(255., 180. + heightValue)) / 255., 1.0);
	}
	else if (textureData.r == 3.0f) {
		// Tree
		FragColor = vec4(vec3(min(255., 34. + heightValue), min(255., 87. + heightValue), min(255., 22. + heightValue)) / 255., 1.0);
	} 
	else {
		FragColor = vec4(0., 0., 1.0, 1.0);
	}
}