#version 300 es

precision highp float;
uniform sampler2D u_vramTexture;
uniform vec2 u_origin;
uniform vec2 u_resolution;
uniform vec2 u_mousePos;
uniform bool u_hovered;
uniform bool u_alpha;
uniform int u_mode;
uniform vec2 u_mouseUV;
uniform vec2 u_cornerTL;
uniform vec2 u_cornerBR;
uniform int u_24shift;
in vec2 fragUV;
out vec4 outColor;
// layout(origin_upper_left) in vec4 gl_FragCoord; // causes some machines to crash on failed assert due to Invalid layout qualifier "origin_upper_left"
uniform bool u_magnify;
uniform float u_magnifyRadius;
uniform float u_magnifyAmount;
const float ridge = 1.5f;

uniform bool u_greyscale;
uniform vec2 u_clut;

const vec4 grey1 = vec4(0.6f, 0.6f, 0.6f, 1.0f);
const vec4 grey2 = vec4(0.8f, 0.8f, 0.8f, 1.0f);

int texelToRaw(in vec4 t) {
    int c = (int(t.r * 31.0f + 0.5f) <<  0) |
            (int(t.g * 31.0f + 0.5f) <<  5) |
            (int(t.b * 31.0f + 0.5f) << 10) |
            (int(t.a) << 15);
    return c;
}

vec4 readTexture(in vec2 pos) {
    vec2 apos = vec2(1024.0f, 512.0f) * pos;
    vec2 fpos = fract(apos);
    ivec2 ipos = ivec2(apos);
    vec4 ret = vec4(0.0f);
    if (pos.x > 1.0f) return ret;
    if (pos.y > 1.0f) return ret;
    if (pos.x < 0.0f) return ret;
    if (pos.y < 0.0f) return ret;

    float scale = 0.0f;
    int p = 0;
    vec4 t = texture(u_vramTexture, pos);
    int c = texelToRaw(t);

    switch (u_mode) {
    case 0:
        {
            ret.a = 1.0f;
            vec4 tb = texture(u_vramTexture, pos - vec2(1.0 / 1024.0f, 0.0f));
            vec4 ta = texture(u_vramTexture, pos + vec2(1.0 / 1024.0f, 0.0f));
            int cb = texelToRaw(tb);
            int ca = texelToRaw(ta);
            switch ((ipos.x + u_24shift) % 3) {
                case 0:
                    ret.r = float((c >> 0) & 0xff) / 255.0f;
                    ret.g = float((c >> 8) & 0xff) / 255.0f;
                    ret.b = float((ca >> 0) & 0xff) / 255.0f;
                    break;
                case 1:
                    if (fpos.x < 0.5f) {
                        ret.r = float((cb >> 0) & 0xff) / 255.0f;
                        ret.g = float((cb >> 8) & 0xff) / 255.0f;
                        ret.b = float((c >> 0) & 0xff) / 255.0f;
                    } else {
                        ret.r = float((c >> 8) & 0xff) / 255.0f;
                        ret.g = float((ca >> 0) & 0xff) / 255.0f;
                        ret.b = float((ca >> 8) & 0xff) / 255.0f;
                    }
                    break;
                case 2:
                    ret.r = float((cb >> 8) & 0xff) / 255.0f;
                    ret.g = float((c >> 0) & 0xff) / 255.0f;
                    ret.b = float((c >> 8) & 0xff) / 255.0f;
                    break;
            }
        }
        break;
    case 1:
        ret = t;
        break;
    case 2:
        scale = 255.0f;
        if (fpos.x < 0.5f) {
            p = (c >> 0) & 0xff;
        } else {
            p = (c >> 8) & 0xff;
        }
        break;
    case 3:
        scale = 15.0f;
        if (fpos.x < 0.25f) {
            p = (c >> 0) & 0xf;
        } else if (fpos.x < 0.5f) {
            p = (c >> 4) & 0xf;
        } else if (fpos.x < 0.75f) {
            p = (c >> 8) & 0xf;
        } else {
            p = (c >> 12) & 0xf;
        }
        break;
    }

    if (u_mode >= 2) {
        if (u_greyscale) {
            ret = vec4(float(p) / scale);
            ret.a = 1.0f;
        } else {
            ret = texture(u_vramTexture, u_clut + vec2(float(p) * 1.0f / 1024.0f, 0.0f));
        }
    } else if (u_greyscale) {
        ret = vec4(0.299, 0.587, 0.114, 0.0f) * ret;
        ret = vec4(ret.r + ret.g + ret.b);
        ret.a = 1.0f;
    }

    return ret;
}

void main() {
    float magnifyAmount = u_magnifyAmount;
    vec2 fragCoord = gl_FragCoord.xy - u_origin;
    vec4 fragColor = readTexture(fragUV.st);
    vec2 magnifyVector = (fragUV.st - u_mouseUV) / u_magnifyAmount;
    vec4 magnifyColor = readTexture(magnifyVector + u_mouseUV);

    float blend = u_magnify ?
        smoothstep(u_magnifyRadius + ridge, u_magnifyRadius, distance(fragCoord, u_mousePos)) :
        0.0f;

    outColor = mix(fragColor, magnifyColor, blend);

    if (u_alpha) {
        int x = int(fragCoord.x);
        int y = int(fragCoord.y);
        int info = (x >> 4) + (y >> 4);
        vec4 back = (info & 1) == 0 ? grey1 : grey2;
        outColor = mix(back, outColor, outColor.a);
    }
    outColor.a = 1.0f;
}
