#ifndef MISC_CAMERA_GLSL
#define MISC_CAMERA_GLSL

layout(set=0, binding=0) uniform UniformCamera
{
	mat4 projection;
    mat4 prevProjection;
    mat4 ortho;
    mat4 prevOrtho;
	mat4 view;
    mat4 prevView;
    mat4 invProjection;
    mat4 invView;
    mat4 stereoProjection[2];
    mat4 prevStereoProjection[2];
    mat4 stereoView[2];
    mat4 prevStereoView[2];
    mat4 invStereoProjection[2];
    mat4 invStereoView[2];
    vec4 frustumVector[4]; // leftTop, rightTop, leftBottom, rightBottom
    vec4 frustumPlane[6]; // left, right, bottom, top, near, far  normal(xyz)--d (ax+by+cz+d = 0)
    vec4 stereoLeftFrustumVector[4];
    vec4 stereoRightFrustumVector[4];
    vec4 stereoLeftFrustumPlane[6];
    vec4 stereoRightFrustumPlane[6];
    vec4 projectionParams; // x = 1 or -1 (-1 if projection is flipped), y = near plane, z = far plane, w = 1/far plane
    vec4 zBufferParams; // x = 1-far/near, y = far/near,  z = x/far, w = y/far
    vec4 pixelSize; // camera's pixelWidth, pixelHeight, 1.0 / pixelWidth, 1.0f / pixelHeight
    vec4 stereoPixelSize;
    vec4 cameraPosition;
    vec4 cameraStereoPosition[2];
    uint frameID;
} camera;

mat4 GetProjection(){ return camera.projection; }

mat4 GetView(){ return camera.view; }

vec3 GetCameraDirection() {
    return vec3(camera.view[0][2], camera.view[1][2], camera.view[2][2]);
}

vec3 GetRayDirection(vec2 uv) {
    vec4 viewPoint = camera.invView * camera.invProjection * vec4(uv.x * 2.0f - 1.0f, (uv.y * 2.0f - 1.0f) * camera.projectionParams.x, -1.0f, 1.0f);
    vec3 ray = (camera.cameraPosition.xyz - viewPoint.xyz / viewPoint.w);
    return normalize(ray);
}

vec4[6] GetFrustumPlanes(int viewIndex) {
    return viewIndex == 0 ? camera.stereoLeftFrustumPlane : camera.stereoRightFrustumPlane;
}

vec4[6] GetFrustumPlanes() {
    return camera.frustumPlane;
}

vec4 GetCameraPosition(int viewIndex) {
    return camera.cameraStereoPosition[viewIndex];
}

float Linear01Depth(float z) {
    z = z * 0.5 + 0.5; // NDC to [0, 1]
    return 1.0 / (camera.zBufferParams.x * z + camera.zBufferParams.y);
}

float Linear01ToProjDepth(float z) {
    z = (1.0 / max(z, 1e-6f) - camera.zBufferParams.y) / camera.zBufferParams.x;
    return z * 2.0 - 1.0;
}

float LinearEyeDepth(float z) {
    z = z * 0.5 + 0.5; // NDC to [0, 1]
    return 1.0 / (camera.zBufferParams.z * z + camera.zBufferParams.w);
}

vec3 ViewSpacePosAtScreenUV(vec2 uv, float depth)
{
    vec3 viewSpaceRay = vec3(camera.invProjection * (vec4(uv.x * 2.0f - 1.0f, (uv.y * 2.0f - 1.0f) * camera.projectionParams.x, -1.0f, 1.0f) * camera.projectionParams.z));
    return viewSpaceRay * Linear01Depth(depth);
}

vec3 WorldSpacePosAtScreenUV(vec2 uv, float depth) 
{
    vec3 viewSpacePos = ViewSpacePosAtScreenUV(uv, depth);
    return (camera.invView * vec4(viewSpacePos, 1.0f)).xyz;
}

vec3 ViewSpacePosAtScreenUVOrtho(vec2 uv, float depth)
{
    vec3 ss = vec3(uv.x, 1.0f - uv.y, depth) * 2.0f - 1.0f;
    return vec3(inverse(camera.ortho) * vec4(ss, 1.0f));
}

vec3 WorldSpacePosAtScreenUVOrtho(vec2 uv, float depth) 
{
    vec3 viewSpacePos = ViewSpacePosAtScreenUVOrtho(uv, depth);
    return (camera.invView * vec4(viewSpacePos, 1.0f)).xyz;
}

#endif
