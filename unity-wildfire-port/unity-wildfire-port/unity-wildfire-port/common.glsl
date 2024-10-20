#define VORTICITY_CONFINEMENT
#define VORTICITY_COEFF 1.0

#define BOX_N 128.0

// Data is organized into 3 "pages" of 128x128x128 voxels.
// Each "page" takes up 2 faces of the 1024x1024 cubemap,
// each face storing 8x8=64 of the 128x128 slices.

vec3 vcubeFromLMN(in int page, in vec3 lmn)
{
    // subtexture within [0,8)^2
    float l = mod(floor(lmn.x), 128.0);
    float tm = mod(l, 8.0);
    float tn = mod((l - tm)/8.0, 8.0);
    vec2 tmn = vec2(tm, tn);

    // mn within [0,128)^2
    vec2 mn = mod(floor(lmn.yz), 128.0);

    // pixel position on 1024x1024 face
    vec2 fragCoord = 128.0*tmn + mn + 0.5;
    vec2 p = fragCoord*(2.0/1024.0) - 1.0;

    vec3 fv;
    if (page == 1) {
        fv = vec3(1.0, p);
    } else if (page == 2) {
        fv = vec3(p.x, 1.0, p.y);
    } else {
        fv = fv = vec3(p, 1.0);
    }

    if (l < 64.0) {
        return fv;
    } else {
        return -fv;
    }
}

void lmnFromVCube(in vec3 vcube, out int page, out vec3 lmn)
{
    // page and parity, and pixel position on 1024x1024 texture
    vec2 p;
    float parity;
    if (abs(vcube.x) > abs(vcube.y) && abs(vcube.x) > abs(vcube.z)) {
        page = 1;
        p = vcube.yz/vcube.x;
        parity = vcube.x;
    } else if (abs(vcube.y) > abs(vcube.z)) {
        page = 2;
        p = vcube.xz/vcube.y;
        parity = vcube.y;
    } else {
        page = 3;
        p = vcube.xy/vcube.z;
        parity = vcube.z;
    }
    vec2 fragCoord = floor((0.5 + 0.5*p)*1024.0);

    // mn within [0,128)^2
    vec2 mn = mod(fragCoord, 128.0);

    // subtexture within [0,8)^2
    vec2 tmn = floor(fragCoord/128.0);

    float lAdd;
    if (parity > 0.0) {
        lAdd = 0.0;
    } else {
        lAdd = 64.0;
    }
    lmn = vec3(tmn.y*8.0 + tmn.x + lAdd, mn);
}