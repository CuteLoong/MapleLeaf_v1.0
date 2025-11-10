#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include <Misc/RayTracingCommon.glsl>
#include <Misc/Constants.glsl>
#include <Misc/Parameters.glsl>
#include <Materials/Fresnel.glsl>
#include <Materials/StandardBSDF.glsl>
#include <Sampling/TinyEncryptionSample.glsl>
#include <Lighting/Lighting.glsl>
#include <Lighting/LTC.glsl>
#include <Utils/ShadingFrame.glsl>
#include <Lighting/EmissiveUniformSampler.glsl>

layout(location = 0) rayPayloadInEXT HitPayLoad prd;
hitAttributeEXT vec3 attribs;

layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) readonly buffer Indices {uint i[]; };

layout(set = 0, binding = 1) uniform accelerationStructureEXT topLevelAS;

layout(set = 0, binding = 4, scalar) uniform UniformGeometry {
	uint64_t vertexAddress;
	uint64_t indexAddress; 
} uniformGeometry;

layout(set=0, binding = 5) uniform UniformScene {
	mat4 shadowMatrix;

	int pointLightsCount;
	int directionalLightsCount;
	int areaLightsCount;
	int emissiveCount;
	int emissiveTriangleCount;
	int skyboxLoaded;
} uniformScene;

layout(set = 0, binding = 6) readonly buffer InstanceDatas
{
    GPUInstanceData instanceData[];
} instanceDatas;

layout(set = 0, binding = 7) buffer MaterialDatas
{
    GPUMaterialData materialData[];
} materialDatas;

layout(set = 0, binding = 8) buffer EmissiveIDs
{
    int emissiveIDs[];
} emissiveIDs;

layout(set=0, binding = 9) buffer BufferPointLights {
	PointLight lights[];
} bufferPointLights;

layout(set=0, binding = 10) buffer BufferDirectionalLights {
	DirectionalLight lights[];
} bufferDirectionalLights;

layout(set=0, binding = 11) buffer BufferAreaLights {
	AreaLight lights[];
} bufferAreaLights;

layout(set = 0, binding = 12) readonly buffer EmissiveTriangleDatas
{
    EmissiveTriangle triangles[];
} emissiveTriangleDatas;

layout(set=0, binding = 13) uniform sampler2D samplerBRDF;
layout(set=0, binding = 14) uniform samplerCube samplerIrradiance;
layout(set=0, binding = 15) uniform samplerCube samplerPrefiltered;

layout(set=0, binding = 16) uniform sampler2D LTC1;
layout(set=0, binding = 17) uniform sampler2D LTC2;

layout(set = 1, binding = 0) uniform sampler2D ImageSamplers[];

void decomposition_2x2_symmetric(mat2 C_spatial, out float L1, out float L2, out vec2 v1, out vec2 v2)
{
	// [ a b ]
    // [ b d ]
    float a = C_spatial[0][0];
    float b = C_spatial[0][1];
    float d = C_spatial[1][1];

	float T = a + d;       // Trace
    float D = a * d - b * b; // Determinant

	float T_over_2 = T * 0.5;
    float discriminant = sqrt(max(T_over_2 * T_over_2 - D, 0.0));
    
    L1 = T_over_2 + discriminant;
    L2 = T_over_2 - discriminant;

    if (abs(b) < 1e-6) {
        v1 = vec2(1.0, 0.0);
        v2 = vec2(0.0, 1.0);
    } else {
        v1 = normalize(vec2(b, L1 - a));
        v2 = normalize(vec2(b, L2 - a));
    }
}

mat4 propagate_transport(mat4 sigma, float dist, float cos_theta)
{
	// [ 1 0 -d 0 ]
    // [ 0 1 0 -d ]
    // [ 0 0 1 0 ]
    // [ 0 0 0 1 ]
	float d = dist / max(cos_theta, 0.01);

	mat4 T_d_paper = mat4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        -d,0, 1, 0,
        0, -d,0, 1
    );

	return T_d_paper * sigma * transpose(T_d_paper);
}

mat4 reflect_brdf(mat4 sigma, float roughness)
{
	// B = [ 0 0 0 0 ]
    //     [ 0 0 0 0 ]
    //     [ 0 0 b_u 0 ]
    //     [ 0 0 0 b_v ]
	float b = 1.0 / max(roughness * roughness, 1e-6);

	mat4 B = mat4(0.0);
    B[2][2] = b;
    B[3][3] = b;

	// V = [0 0 1 0; 0 0 0 1]
    // U = transpose(V)
    mat2 V_Sigma_U = mat2(sigma[2][2], sigma[2][3], sigma[3][2], sigma[3][3]);
    mat2 B_2x2 = mat2(B[2][2], B[2][3], B[3][2], B[3][3]);
    
    mat2 mid_inv = inverse(B_2x2 + V_Sigma_U);
    
    mat4 Sigma_U = mat4(
        0, 0, sigma[0][2], sigma[0][3],
        0, 0, sigma[1][2], sigma[1][3],
        0, 0, sigma[2][2], sigma[2][3],
        0, 0, sigma[3][2], sigma[3][3]
    );
    
    mat4 V_Sigma = mat4(
        0, 0, 0, 0,
        0, 0, 0, 0,
        sigma[2][0], sigma[2][1], sigma[2][2], sigma[2][3],
        sigma[3][0], sigma[3][1], sigma[3][2], sigma[3][3]
    );
    
    mat4 U_mid_inv_V = mat4(0.0);
    U_mid_inv_V[2][2] = mid_inv[0][0];
    U_mid_inv_V[2][3] = mid_inv[0][1];
    U_mid_inv_V[3][2] = mid_inv[1][0];
    U_mid_inv_V[3][3] = mid_inv[1][1];

	return sigma - Sigma_U * U_mid_inv_V * V_Sigma;
}

uint GetEmissiveTriangleCount()
{
	uint triCount = 0;
	for(int i = 0; i < uniformScene.emissiveCount; i++)
	{
		int instanceID = emissiveIDs.emissiveIDs[i];
		GPUInstanceData instanceData = instanceDatas.instanceData[instanceID];
		uint indexCount = instanceData.indexCount;
		triCount += indexCount / 3;
	}
	return triCount;
}

bool instanceIsLight(int instanceID)
{
	for(int i = 0; i < uniformScene.emissiveCount; i++)
	{
		if(emissiveIDs.emissiveIDs[i] == instanceID) return true;
	}
	return false;
}

bool sampleLight(vec3 u, const vec3 posW, const vec3 normalW, const bool upperHemisphere, out TriangleLightSample ls)
{
	float uLight = u.x;
	uint triangleCount = uniformScene.emissiveTriangleCount;
	if(triangleCount == 0) return false;
	uint idx = min(uint(uLight * float(triangleCount)), triangleCount - 1);
	float triangleSelectionPdf = 1.0f / float(triangleCount);
	EmissiveTriangle tri = emissiveTriangleDatas.triangles[idx];

	vec2 uTriangle = u.yz;
	const vec3 barycentric = sample_triangle(uTriangle);
	ls.posW = vec3(tri.posW[0] * barycentric.x + tri.posW[1] * barycentric.y + tri.posW[2] * barycentric.z);
	vec3 toLight = ls.posW - posW;
	const float distSqr = max(dot(toLight, toLight), FLT_MIN);
	ls.dist = sqrt(distSqr);
	ls.dir = toLight / ls.dist;
	ls.normalW = tri.normal;

	float cosTheta = dot(ls.normalW, -ls.dir);
	if(upperHemisphere && cosTheta <= 0.0f) return false;
	else cosTheta = abs(cosTheta);

	vec2 texCoords = tri.texCoords[0] * barycentric.x + tri.texCoords[1] * barycentric.y + tri.texCoords[2] * barycentric.z;
	uint materialID = tri.materialID;
	GPUMaterialData material = materialDatas.materialData[materialID];
	vec4 emissiveColor = material.emissiveColor;

	ls.Le = emissiveColor.rgb * material.emissiveIntensity * M_1_PI;
	float denom = max(FLT_MIN, cosTheta * tri.area);
	ls.pdf = distSqr / denom * triangleSelectionPdf;
	ls.uv = texCoords;

	ls.Le /= ls.pdf;
	return true;
}

float evalMIS(float n0, float p0, float n1, float p1)
{
	float q0 = n0 * p0;
    float q1 = n1 * p1;
    return q0 / (q0 + q1);
}

void main()
{
	GPUInstanceData instanceData = instanceDatas.instanceData[gl_InstanceCustomIndexEXT];
	GPUMaterialData material = materialDatas.materialData[instanceData.materialID];
	Vertices vertices = Vertices(uniformGeometry.vertexAddress);
	Indices indices = Indices(uniformGeometry.indexAddress);

	uint indexOffset = instanceData.indexOffset;
	uint vertexOffset = instanceData.vertexOffset;

	vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);	
	ivec3 index = ivec3(indices.i[indexOffset + gl_PrimitiveID * 3], indices.i[indexOffset + gl_PrimitiveID * 3 + 1], indices.i[indexOffset + gl_PrimitiveID * 3 + 2]);
	Vertex v0 = vertices.v[vertexOffset + index.x];
	Vertex v1 = vertices.v[vertexOffset + index.y];
	Vertex v2 = vertices.v[vertexOffset + index.z];

	vec3 position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
	vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
	vec4 tangent = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
	tangent = vec4(normalize(vec3(tangent.xyz * gl_WorldToObjectEXT)), sign(tangent.w));
	vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

	vec3 worldPosition = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0f));
	vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

	// Propagate the covariance matrix through the transport
	float hit_dist = gl_HitTEXT;
	float cos_theta_in = abs(dot(gl_WorldRayDirectionEXT, worldNormal));
	mat4 propagated_covariance = propagate_transport(prd.covariance, hit_dist, cos_theta_in);

	vec3 N = normalize(worldNormal);
	vec3 T = normalize(tangent.xyz);
	T = normalize(T - dot(T, N) * N);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	mat2 Sigma_spatial = mat2(propagated_covariance[0][0], propagated_covariance[0][1], propagated_covariance[1][0], propagated_covariance[1][1]);

	float L1, L2;
	vec2 tv1, tv2;
    decomposition_2x2_symmetric(Sigma_spatial, L1, L2, tv1, tv2);

	vec3 dp_dx_tangent_space = vec3( sqrt(max(L1, 0.0)) * tv1, 0.0);
    vec3 dp_dy_tangent_space = vec3( sqrt(max(L2, 0.0)) * tv2, 0.0);

	vec3 dp_dx_world = TBN * dp_dx_tangent_space;
    vec3 dp_dy_world = TBN * dp_dy_tangent_space;

	vec3 E1 = v1.position - v0.position;
	vec3 E2 = v2.position - v0.position;
	vec2 duv1 = v1.texCoord - v0.texCoord;
	vec2 duv2 = v2.texCoord - v0.texCoord;

	vec3 dp_du_tex;
    vec3 dp_dv_tex;
    float A, B_geom, C_geom, invDet_geom;
    float det_uv = duv1.x * duv2.y - duv1.y * duv2.x;

    if (abs(det_uv) < 1e-4) { 
        dp_du_tex = T;
        dp_dv_tex = B;
        A = 1.0;
        B_geom = 0.0;
        C_geom = 1.0;
        invDet_geom = 1.0;
    } else {
        float invDet_uv = 1.0 / det_uv; 
        vec3 dp_du_obj = invDet_uv * (duv2.y * E1 - duv1.y * E2);
        vec3 dp_dv_obj = invDet_uv * (-duv2.x * E1 + duv1.x * E2);
        dp_du_tex = (gl_ObjectToWorldEXT * vec4(dp_du_obj, 0.0)).xyz;
        dp_dv_tex = (gl_ObjectToWorldEXT * vec4(dp_dv_obj, 0.0)).xyz;
        A = dot(dp_du_tex, dp_du_tex);
        B_geom = dot(dp_du_tex, dp_dv_tex);
        C_geom = dot(dp_dv_tex, dp_dv_tex);
        invDet_geom = 1.0 / max(A * C_geom - B_geom * B_geom, 1e-7);
    }

	vec2 ddx = vec2( // (du/dx, dv/dx)
        invDet_geom * (C_geom * dot(dp_du_tex, dp_dx_world) - B_geom * dot(dp_dv_tex, dp_dx_world)),
        invDet_geom * (A * dot(dp_dv_tex, dp_dx_world) - B_geom * dot(dp_du_tex, dp_dx_world))
    );
    vec2 ddy = vec2( // (du/dy, dv/dy)
        invDet_geom * (C_geom * dot(dp_du_tex, dp_dy_world) - B_geom * dot(dp_dv_tex, dp_dy_world)),
        invDet_geom * (A * dot(dp_dv_tex, dp_dy_world) - B_geom * dot(dp_du_tex, dp_dy_world))
    );

	// 
	vec4 baseColor = material.baseColor;
	float metallic = material.metallic;
	float roughness = material.roughness;

	int baseColorTex = material.baseColorTex;
	int normalTex = material.normalTex;
	int materialTex = material.materialTex;

	// if(baseColorTex != -1) baseColor = texture(ImageSamplers[baseColorTex], texCoord);
	if(baseColorTex != -1) baseColor = textureGrad(ImageSamplers[baseColorTex], texCoord, ddx, ddy); 

	if(materialTex != -1) {
		vec4 textureMaterial = textureGrad(ImageSamplers[materialTex], texCoord, ddx, ddy);
        metallic *= textureMaterial.b;
        roughness *= textureMaterial.g;
	}
	if(normalTex != -1) {
		vec3 tangentNormal = textureGrad(ImageSamplers[normalTex], texCoord, ddx, ddy).rgb * 2.0f - 1.0f;
		worldNormal = normalize(TBN * tangentNormal);
	}
	Frame frame;
	initFrame(frame, worldNormal, tangent);

	StandardBSDFParameters params;

	vec3 wiLocal = normalize(toLocal(frame, -normalize(gl_WorldRayDirectionEXT))); // view vector in local space
	StandardBSDFInit(params, baseColor.rgb, 0.5f, roughness, metallic, wiLocal);

	vec3 Lo = vec3(0.0f);

	// if hit light source, return light color directly
	if(instanceIsLight(gl_InstanceCustomIndexEXT))
	{
		Lo = material.emissiveColor.rgb * material.emissiveIntensity * M_1_PI;
		prd.radiance = Lo * prd.accBrdf;
		return;
	}
	
	vec3 u = vec3(TinyEncryptionRandom(prd.randomSeed), TinyEncryptionRandom(prd.randomSeed), TinyEncryptionRandom(prd.randomSeed));
	bool brdfSampled = false;
	vec3 wo;
	float pdf = 0.0f;
	vec3 weight;
	brdfSampled = bsdf_sample(params, u, wiLocal, wo, pdf, weight);

	vec3 u_2 = vec3(TinyEncryptionRandom(prd.randomSeed), TinyEncryptionRandom(prd.randomSeed), TinyEncryptionRandom(prd.randomSeed));

	TriangleLightSample ls;
	bool lightSampled = sampleLight(u_2, worldPosition, worldNormal, false, ls);

	// nee
	if(lightSampled)
	{
		// shadow ray
		vec3  origin    = worldPosition;
		vec3  direction = ls.dir;  // vector to light
		float tMin      = 0.01f;
		float tMax      = ls.dist;

		// Initializes a ray query object but does not start traversal
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, tMin, direction, tMax);

		while(rayQueryProceedEXT(rayQuery)) {}

		if(rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT)
		{
			vec3 woLocal = toLocal(frame, ls.dir);
			float scatterPdf = evalPdf(params, wiLocal, woLocal);
			vec3 f = eval(params, wiLocal, woLocal);
			float MISweight = evalMIS(1.0f, ls.pdf, 1.0f, scatterPdf);

			prd.radiance = f * ls.Le * MISweight * prd.accBrdf;
		}
	}

	if(brdfSampled)
	{
		prd.covariance = reflect_brdf(propagated_covariance, roughness * roughness);
		prd.nextOrigin = vec4(worldPosition, 1.0f);
		prd.nextDir = vec4(toWorld(frame, wo), 0.0f);

		prd.accBrdf *= weight;
		prd.done = 0;
	}

}
