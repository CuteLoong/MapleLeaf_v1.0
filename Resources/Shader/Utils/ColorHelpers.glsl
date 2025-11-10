#ifndef COLOR_HELPERS_GLSL
#define COLOR_HELPERS_GLSL

float luminance(vec3 rgb)
{
    return dot(rgb, vec3(0.2126f, 0.7152f, 0.0722f));
}

#endif // COLOR_HELPERS_GLSL