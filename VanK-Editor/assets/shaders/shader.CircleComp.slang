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

[[vk::binding(3, 1)]]  
StructuredBuffer<CircleInstance> circles;

[[vk::binding(2, 1)]]  
RWStructuredBuffer<CircleVertex> VertexBuffer;

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

    CircleInstance circle = circles[n];

    // Build rotation matrix from Euler angles
    float3x3 rotMatrix = RotationMatrix(circle.Rotation);

    for (uint i = 0; i < 4; i++)
    {
        // Local position in 2D quad plane scaled by Size and Scale
        float2 localPos2D = quadOffsets[i] * 2.0f;

        // Extend to 3D local position (Z=0)
        float3 localPos3D = float3(localPos2D, 0);

        // Scale world position
        float2 worldOffset2D = quadOffsets[i] * circle.Size * circle.Scale.xy;

        // Rotate world position
        float3 rotatedOffset = mul(rotMatrix, float3(worldOffset2D, 0));

        // Translate to world position
        float3 worldPos = circle.WorldPosition + rotatedOffset;

        // Fill vertex
        CircleVertex v;
        v.WorldPosition = float4(worldPos, 1.0f);
        v.LocalPosition = localPos3D;
        v.Color = circle.Color;
        v.Thickness = circle.Thickness;
        v.Fade = circle.Fade;
        v.EntityID = circle.EntityID;

        // Write vertex to output buffer (4 vertices per quad)
        VertexBuffer[n * 4u + i] = v;
    }
}