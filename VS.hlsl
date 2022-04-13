#pragma pack_matrix(row_major)
#define MAX_SUBMESH_PER_DRAW 1024

[[vk::push_constant]]
cbuffer MESH_INDEX
{
    uint mesh_ID;
};

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
    float4 SunDirection, SunColor, SunAmbient, CamPos;
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
    float3 PosW : WORLD;
};
// TODO: Part 4b

StructuredBuffer<SHADER_MODEL_DATA> SceneData;
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = float4(input.Pos, 1);
    output.Norm = mul(float4(input.Norm, 0), SceneData[0].matricies[mesh_ID]).xyz;
    output.Uvw = input.Uvw;
    
    output.Pos = mul(output.Pos, SceneData[0].matricies[mesh_ID]);
    output.PosW = output.Pos.xyz;
    
    output.Pos = mul(output.Pos, SceneData[0].ViewMatrix);
    output.Pos = mul(output.Pos, SceneData[0].ProjectionMatrix);

    return output;
}