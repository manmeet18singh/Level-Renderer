#pragma pack_matrix(row_major)
#define MAX_SUBMESH_PER_DRAW 1024

struct OBJ_ATTRIBUTES
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
};

struct SHADER_MODEL_DATA
{
    float4 SunDirection, SunColor;
    matrix ViewMatrix, ProjectionMatrix;

    matrix matricies[MAX_SUBMESH_PER_DRAW];
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Uvw : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float3 Uvw : TEXCOORD;
};
// TODO: Part 4b

StructuredBuffer<SHADER_MODEL_DATA> SceneData;
float4 main(PS_INPUT input) : SV_TARGET
{
    return float4(0.75f, 0.75f, 0.25f, 0); // TODO: Part 1a
	// TODO: Part 3a
    //return SceneData[0].materials[0];
	// TODO: Part 4c
	// TODO: Part 4g (half-vector or reflect method your choice)
}