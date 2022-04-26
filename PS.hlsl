#pragma pack_matrix(row_major)
#define MAX_SUBMESH_PER_DRAW 1024

[[vk::push_constant]]
cbuffer MESH_INDEX
{
    uint model_ID;
    uint material_ID;
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

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Uvw : TEXCOORD;
    float3 Norm : NORMAL;
    float3 PosW : WORLD;
};
// TODO: Part 4b

StructuredBuffer<SHADER_MODEL_DATA> SceneData;
float4 main(PS_INPUT input) : SV_TARGET
{
    input.Norm = normalize(input.Norm);
    
    float lightRatio = clamp(dot(-SceneData[0].SunDirection.xyz, input.Norm), 0, 1);
    
    lightRatio += SceneData[0].SunAmbient;
    
	// TODO: Part 3a
	// TODO: Part 4c
    float3 finalLight = lightRatio * SceneData[0].SunColor.xyz * SceneData[0].materials[material_ID].Kd;
  //  
	 //TODO: Part 4g (half-vector or reflect method your choice)  
    float3 viewDir = normalize(SceneData[0].CamPos.xyz - input.PosW.xyz);
    float3 halfVector = normalize((-SceneData[0].SunDirection.xyz) + viewDir);
    float intensity = max(pow(clamp(dot(input.Norm, halfVector), 0, 1), SceneData[0].materials[material_ID].Ns), 0);
    float3 reflectedLight = SceneData[0].SunColor.xyz * SceneData[0].materials[material_ID].Ks * intensity;

    finalLight += reflectedLight;
    return saturate(float4(finalLight, 0));
}