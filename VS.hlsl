#define MAX_SUBMESH_PER_DRAW 1024
struct SHADER_MODEL_DATA
{
    float4 SunDirection, SunColor;
    matrix ViewMatrix, ProjectionMatrix;

    matrix matricies[MAX_SUBMESH_PER_DRAW];
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
};

[[vk::push_constant]] 
cbuffer SHADER_VARS
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD1;
};
// TODO: Part 4b

StructuredBuffer<SHADER_MODEL_DATA> SceneData;
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
   /* output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Norm = mul(input.Norm, (float3x3) World);
    output.Tex = input.Tex;*/

    output.Pos = float4(input.Pos, 1);
    output.Norm = input.Norm;
    output.Tex = input.Tex;

    return output;
}