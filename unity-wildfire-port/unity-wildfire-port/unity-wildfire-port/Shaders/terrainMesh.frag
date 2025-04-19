#version 410 core

// Texture coordinates from the Tessellation Evaluation Shader
in vec2 TexCoord;

// Output color of the fragment
out vec4 FragColor;

// The PNG texture we are reading from.
uniform sampler2D landscapeTexture;
uniform sampler2D wildfireTexture;

void main()
{
    vec2 uv = TexCoord;

	vec4 textureData = texture(wildfireTexture, uv);

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
		// Tree 1
		FragColor = vec4(vec3(min(255., 32. + heightValue), min(255., 99. + heightValue), min(255., 84. + heightValue)) / 255., 1.0);
	} 
	else if (textureData.r == 4.0f) {
		// Tree 2	
		FragColor= vec4(vec3(min(255., 66. + heightValue), min(255., 143. + heightValue), min(255., 30. + heightValue)) / 255., 1.0);
	}
	else if (textureData.r == 5.0f) {
		// Tree 3
		FragColor = vec4(vec3(min(255., 185. + heightValue), min(255., 209. + heightValue), min(255., 50. + heightValue)) / 255., 1.0);
	}
	else {
		FragColor = vec4(0., 0., 1.0, 1.0);
	}
}