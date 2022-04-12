// TODO: Part 2b
#define MAX_SUBMESH_PER_DRAW 1024
struct SHADER_MODEL_DATA
{
    float4 SunDirection, SunColor;
    matrix ViewMatrix, ProjectionMatrix;

    matrix matricies[MAX_SUBMESH_PER_DRAW];
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
};

// TODO: Part 4g
// TODO: Part 2i
// TODO: Part 3e
// an ultra simple hlsl pixel shader
// TODO: Part 4b
StructuredBuffer<SHADER_MODEL_DATA> SceneData;
float4 main() : SV_TARGET
{
    return float4(0.75f, 0.75f, 0.25f, 0); // TODO: Part 1a
	// TODO: Part 3a
	// TODO: Part 4c
	// TODO: Part 4g (half-vector or reflect method your choice)
}