//------------------------------------------------------------------------------
// Compute shader demonstrating vertex transformation and buffer device address usage
// This shader rotates vertices at different speeds based on their index
//------------------------------------------------------------------------------

#include "shader_io.h"

//------------------------------------------------------------------------------
// Resource Bindings
//------------------------------------------------------------------------------

// Output texture that can be both read and written
// Note: RWTexture2D is equivalent to Vulkan's storage image
[[vk::binding(0)]] 
RWTexture2D<float4> outImage;

// Push constants for frequently updated data
// Note: Using push constants is more efficient than constant buffers for small, frequently changing data
[[vk::push_constant]]  
ConstantBuffer<PushConstantCompute> pushConst;

[[vk::binding(1, 1)]]  
StructuredBuffer<QuadInstance> quads;

[[vk::binding(2, 1)]]  
RWStructuredBuffer<Vertex> VertexBuffer;

//------------------------------------------------------------------------------
// Compute Shader Entry Point
//------------------------------------------------------------------------------

// Helper: Build rotation matrix from Euler angles (XYZ order)
float3x3 RotationMatrix(float3 rotation)
{
    float cx = cos(rotation.x);
    float sx = sin(rotation.x);
    float cy = cos(rotation.y);
    float sy = sin(rotation.y);
    float cz = cos(rotation.z);
    float sz = sin(rotation.z);

    float3x3 rotX = float3x3(
        1,  0,   0,
        0, cx, -sx,
        0, sx,  cx
    );
    float3x3 rotY = float3x3(
        cy,  0, sy,
        0,   1,  0,
        -sy, 0, cy
    );
    float3x3 rotZ = float3x3(
        cz, -sz, 0,
        sz,  cz, 0,
        0,   0,  1
    );

    return mul(rotZ, mul(rotY, rotX)); // R = Rz * Ry * Rx
}

// Predefined local quad corner positions (centered at origin)
static const float2 quadOffsets[4] = {
    float2(-0.5f, -0.5f),
    float2( 0.5f, -0.5f),
    float2( 0.5f,  0.5f),
    float2(-0.5f,  0.5f)
};

// Declare this as a compute shader with [shader("compute")]
// numthreads specifies the compute shader thread group size:
// - 256 threads in X dimension
// - 1 thread in Y dimension
// - 1 thread in Z dimension
[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint n = GlobalInvocationID.x;
    if (n >= pushConst.numVertex) return;

    QuadInstance sprite = quads[n];

    // Build rotation matrix from Euler angles
    float3x3 rotMatrix = RotationMatrix(sprite.Rotation);

    for (uint i = 0; i < 4; i++)
    {
        // Local position in 2D quad plane scaled by Size and Scale
        float2 localPos2D = quadOffsets[i] * sprite.Size * sprite.Scale.xy;

        // Extend to 3D local position (Z=0)
        float3 localPos3D = float3(localPos2D.x, localPos2D.y, 0);

        // Rotate local position
        float3 rotatedPos = mul(rotMatrix, localPos3D);

        // Translate to world position
        float3 worldPos = sprite.Position + rotatedPos;

        // Fill vertex
        Vertex v;
        v.Position = float4(worldPos, 1.0f);

        // Calculate UVs based on tilePosition and tileSize, scaled by tileMultiplier * tilingfactor
        //maybe in the future multiply before putting to shader so tiltingfacto doesnt have to be a attribute
        v.Texcoord = (quadOffsets[i] + 0.5f) * sprite.TilingFactor;

        v.Color = sprite.Color;
        v.EntityID = sprite.EntityID;
        v.TextureID = sprite.TextureID;

        // Write vertex to output buffer (4 vertices per quad)
        VertexBuffer[n * 4u + i] = v;
    }
}
