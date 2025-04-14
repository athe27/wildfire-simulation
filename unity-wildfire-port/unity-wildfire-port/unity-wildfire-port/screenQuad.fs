#version 430 core
out
vec4 FragColor;

in
vec2 TexCoords;

uniform vec3 m_size;
uniform sampler3D velocityDensityTexture;
uniform sampler3D pressureTempPhiReactionTexture;
uniform sampler3D curlObstaclesTexture;
uniform float iTime;
uniform vec3 iResolution; // x: width, y: height, z: aspect ratio
uniform bool mouseDown;
uniform vec2 mousePos;

#define FIXED_UP vec3(0.0, 1.0, 0.0)
#define TAN_HALF_FOVY 0.5773502691896257
#define CAM_Z_NEAR 0.1
#define CAM_Z_FAR 50.0

#define BOX_MIN -m_size / max(max(m_size.x, m_size.y), m_size.z)
#define BOX_MAX m_size / max(max(m_size.x, m_size.y), m_size.z)

#define EPS 0.001

mat4 getClipToWorld(float aspectWoverH, vec3 nvCamFw)
{
    mat4 clipToEye = mat4(
        aspectWoverH * TAN_HALF_FOVY, 0.0, 0.0, 0.0,
        0.0, TAN_HALF_FOVY, 0.0, 0.0,
        0.0, 0.0, 0.0, (CAM_Z_NEAR - CAM_Z_FAR) / (2.0 * CAM_Z_NEAR * CAM_Z_FAR),
        0.0, 0.0, -1.0, (CAM_Z_NEAR + CAM_Z_FAR) / (2.0 * CAM_Z_NEAR * CAM_Z_FAR)
    );

    vec3 nvCamRt = normalize(cross(nvCamFw, FIXED_UP));
    vec3 nvCamUp = cross(nvCamRt, nvCamFw);
    mat4 eyeToWorld = mat4(
         nvCamRt, 0.0,
         nvCamUp, 0.0,
        -nvCamFw, 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    return eyeToWorld * clipToEye;
}

void boxClip(
    in vec3 boxMin, in vec3 boxMax,
    in vec3 p, in vec3 v,
    out vec2 tRange, out float didHit
)
{
    //for each coord, clip tRange to only contain t-values for which p+t*v is in range
    vec3 tb0 = (boxMin - p) / v;
    vec3 tb1 = (boxMax - p) / v;
    vec3 tmin = min(tb0, tb1);
    vec3 tmax = max(tb0, tb1);

    //t must be > tRange.s and each tmin, so > max of these; similar for t1
    tRange = vec2(
        max(max(tmin.x, tmin.y), tmin.z),
        min(min(tmax.x, tmax.y), tmax.z)
    );

    //determine whether ray intersects the box
    didHit = step(tRange.s, tRange.t);
}

vec3 lmnFromWorldPos(vec3 p)
{
    vec3 uvw = (p - BOX_MIN) / (BOX_MAX - BOX_MIN);
    return (uvw * m_size);
}

vec4 readVelocityDensity(vec3 voxelCoords)
{
    vec3 texCoords = voxelCoords / m_size;
    return texture(velocityDensityTexture, texCoords);
}

vec4 readPressureTempPhiReaction(vec3 voxelCoords)
{
    vec3 texCoords = voxelCoords / m_size;
    return texture(pressureTempPhiReactionTexture, texCoords);
}

vec4 readCurlObstacles(vec3 voxelCoords)
{
    vec3 texCoords = voxelCoords / m_size;
    return texture(curlObstaclesTexture, texCoords);
}

void boxFromLMN(in vec3 lmn, out vec3 boxMin, out vec3 boxMax)
{
    vec3 boxSize = (BOX_MAX - BOX_MIN) / m_size;

    boxMin = BOX_MIN + (floor(lmn) / m_size) * (BOX_MAX - BOX_MIN);
    boxMax = boxMin + boxSize;
}

float unmix(float a, float b, float x)
{
    return (x - a) / (b - a);
}

vec3 colormapInferno(float t, float h)
{
    float value = pow(t, 1.0 + h * 10.0);
    vec3 target = vec3(0.05, 0.1, 0.2);
    return smoothstep(vec3(0.0), target, vec3(value));

    return clamp(vec3(
        1.0 - (t - 1.0) * (t - 1.0),
        t * t,
        t * (3.0 * t - 2.0) * (3.0 * t - 2.0)
    ) * 0.5 * vec3(3., 1.5, 1.1), 0, 1);
}

vec3 colormapInferno2(float t)
{
    t = 1 - t;
    return clamp(vec3(1.5 * t, 1.5 * t * t * t, t * t * t * t * t * t), 0, 1);
}

void march(
    in vec3 p, in vec3 nv,
    out vec4 color
)
{
    vec2 tRange;
    float didHitBox;
    boxClip(BOX_MIN, BOX_MAX, p, nv, tRange, didHitBox);

    color = vec4(0.0);
    if (didHitBox < 0.5)
    {
        return;
    }

    float t = tRange.s;
    for (int i = 0; i < 800; i++)
    {
		// Get voxel data
        vec3 lmn = lmnFromWorldPos(p + (t + EPS) * nv);
        
        vec4 velocityDensityData = readVelocityDensity(lmn);
        vec4 pressureTempPhiReactionData = readPressureTempPhiReaction(lmn);

        vec3 curlV = readCurlObstacles(lmn).xyz;

        float normalizedDensity = unmix(0.5, 3.0, velocityDensityData.w + pressureTempPhiReactionData.w);
        float normalizedSpeed = pow(unmix(0.0, 10.0, length(velocityDensityData.xyz)), 0.5);
        float normalizedVorticity = clamp(pow(length(curlV), 0.5), 0.0, 1.0);

        float normalizedTemperature = 0.5 * normalizedSpeed + 0.5 * normalizedVorticity;
        vec3 cbase = colormapInferno(normalizedTemperature, 1 - (lmn.y / m_size.y));
        float calpha = pow(normalizedSpeed, 3.0) * .2;
        //vec3 fireColor = mix(vec3(normalizedSpeed), cbase, normalizedSpeed);

        vec4 ci = vec4(cbase, 1.0) * calpha;

        // Determine path to next voxel
        vec3 curBoxMin, curBoxMax;
        boxFromLMN(lmn, curBoxMin, curBoxMax);

        vec2 curTRange;
        float curDidHit;
        boxClip(curBoxMin, curBoxMax, p, nv, curTRange, curDidHit);

        // Adjust alpha for distance through the voxel
        ci *= clamp((curTRange.t - curTRange.s) * 15.0, 0.0, 1.0);

        // Accumulate color
        color = vec4(
            color.rgb + (1.0 - color.a) * ci.rgb,
            color.a + ci.a - color.a * ci.a
        );
        
        // Move up to next voxel
        t = curTRange.t;
        if (t + EPS > tRange.t || color.a > 1.0)
        {
            break;
        }
    }
}

void march2(
    in vec3 p, in vec3 nv,
    out vec4 color
)
{
    vec2 tRange;
    float didHitBox;
    boxClip(BOX_MIN, BOX_MAX, p, nv, tRange, didHitBox);

    color = vec4(0.0);

    if (didHitBox < 0.5)
    {
        return;
    }

    if (tRange.x < 0.0)
    {
        tRange.x = 0.0;
    }

    vec3 rayStart = p + nv * tRange.x;
    vec3 rayStop = p + nv * tRange.y;

    vec3 start = rayStart;
    float dist = distance(rayStop, rayStart);
    float stepSize = dist / 64.0;

    vec3 ds = normalize(rayStop - rayStart) * stepSize;
    float fireAlpha = 1.0;
    float smokeAlpha = 1.0;

    for (int i = 0; i < 64; i++)
    {
        vec3 voxelCoord = lmnFromWorldPos(start);

        float D = readVelocityDensity(voxelCoord).w;
        float R = readPressureTempPhiReaction(voxelCoord).w;

        fireAlpha *= 1.0 - clamp(R * stepSize * 40.0, 0.0, 1.0);
        smokeAlpha *= 1.0 - clamp(D * stepSize * 100.0, 0.0, 1.0);
        			
        if (fireAlpha <= 0.01 && smokeAlpha <= 0.01)
        {
            break;
        }

        start += ds;
    }

    vec4 smoke = vec4(vec3(0), 1) * (1.0 - smokeAlpha);
    vec4 fire = vec4(colormapInferno2(fireAlpha), 1.0) * (1.0 - fireAlpha);

    color = fire;
    color = vec4(color.rgb + (1.0 - color.a) * smoke.xyz, color.a + smoke.a - color.a * smoke.a);
}

void main()
{
    vec2 uv = TexCoords;

    //vec2 mouseAng = vec2(-iTime*0.27, 0.5*3.14159 + 0.6*sin(iTime*0.21));
    //vec3 camPos = 2.5 * (
    //    sin(mouseAng.y) * vec3(cos(2.0*mouseAng.x), 0.0, sin(2.0*mouseAng.x)) +
    //    cos(mouseAng.y) * vec3(0.0, 1.0, 0.0)
    //);

    int mouseMix = 0;
    if (mouseDown == true)
    {
        mouseMix = 1;
    }

    vec2 mouseAng = mix(
        vec2(0, 0.5 * 3.14159),
        3.14159 * vec2(mousePos.x, iResolution.y - mousePos.y) / iResolution.xy,
        mouseMix
    );

    vec3 camPos = 2.5 * (
        sin(mouseAng.y) * vec3(cos(2.0 * mouseAng.x), 0.0, sin(2.0 * mouseAng.x)) +
        cos(mouseAng.y) * vec3(0.0, 1.0, 0.0)
    );
    vec3 lookTarget = vec3(0.0);

    vec3 nvCamFw = normalize(lookTarget - camPos);
    mat4 clipToWorld = getClipToWorld(iResolution.x / iResolution.y, nvCamFw);

    vec4 vWorld = clipToWorld * vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec3 nvCamDir = normalize(vWorld.xyz / vWorld.w);

    vec3 bgColor = vec3(1.0);

    vec4 finalColor;
    march2(camPos, nvCamDir, finalColor);
    FragColor = vec4(finalColor.rgb + (1.0 - finalColor.a) * bgColor, 1.0);
}