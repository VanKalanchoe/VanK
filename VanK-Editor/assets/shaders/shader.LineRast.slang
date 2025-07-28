//------------------------------------------------------------------------------
// This shader demonstrates key Slang features and syntax while implementing
// a textured triangle renderer with dynamic point overlay effects
//------------------------------------------------------------------------------

#include "shader_io.h"

//------------------------------------------------------------------------------
// Input Structures
//------------------------------------------------------------------------------

// Define the vertex attributes that will be provided by the vertex buffer
// Note: In Slang, we use semantic annotations (: POSITION, etc.) to specify
// how these fields map to vertex buffer data
struct VertexInput
{
    [[vk::location(LVLinePosition)]] // Vulkan layout(location = 0)
    float3 Position : POSITION;  // Vertex position in model space
    [[vk::location(LVLineColor)]]    // Vulkan layout(location = 1)
    float4 Color : COLOR;        // Vertex color
    [[vk::location(LVLineEntityID)]] 
    int EntityID : USER_DEFINED_SEMANTIC;
};

// Define the data that will be interpolated and passed from vertex to fragment shader
// Note: SV_Position is a special semantic in Slang/HLSL for the clip-space position
struct VertexOutput
{
    float4 Position : SV_Position; // Clip space position (required)
    float4 Color : TEXCOORD0;      // Interpolated vertex color
    int EntityID : TEXCOORD1;
};

// Define the final output of the fragment shader
// Note: SV_TARGET specifies this is the color output to the render target
struct PixelOutput
{
    float4 Color : SV_TARGET0; // Final color output (RGBA)
    int EntityID : SV_TARGET1; // Final color output (RGBA)
   /*  float Depth : SV_Depth; */
};

//------------------------------------------------------------------------------
// Resource Bindings
//------------------------------------------------------------------------------

// Bind an array of textures
// Note: [[vk::binding(x,y)]] specifies binding=x in descriptor set=y for Vulkan
[[vk::binding(LBindTextures, LSetTextures)]]
Sampler2D inTexture[]; // Array of textures in descriptor set 0, binding 0

// Bind our scene information constant buffer
// Note: ConstantBuffer is similar to Vulkan's UBO (Uniform Buffer Object)
[[vk::binding(LBindSceneInfo, LSetScene)]]
ConstantBuffer<SceneInfo> sceneInfo; // Scene data in descriptor set 0, binding 1

// Bind push constants for frequently updated data
// Note: Push constants are fastest way to send small amount of data to shader
[[vk::push_constant]]
ConstantBuffer<PushConstant> pushConst;

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

// Note: [shader("vertex")] is Slang's way of marking the vertex shader entry point
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    /* // Start with the input position
    vec3 pos = input.Position;

    // Correct for screen aspect ratio to prevent stretching
    float aspectRatio = sceneInfo.resolution.y / sceneInfo.resolution.x;
    pos.x *= aspectRatio; */

    VertexOutput output;
    output.Position  = mul(sceneInfo.MatrixTransform, float4(input.Position, 1.0)); // Convert to homogeneous coordinates
    output.Position.y *= -1.0f; // Flip to compensate for Vulkan's Y-down NDC
    output.Color     = input.Color;      // Pass through the vertex color
    output.EntityID  = input.EntityID;
    return output;
}

//------------------------------------------------------------------------------
// Fragment/Pixel Shader
//------------------------------------------------------------------------------

/* float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0;
    return ((2.0 * near * far) / (far + near - z * (far - near))) / far;
}
 */
// Note: [shader("pixel")] is Slang's way of marking the fragment shader entry point
[shader("pixel")]
PixelOutput fragmentMain(VertexOutput input)
{
    PixelOutput output;

    output.Color = input.Color;

    output.EntityID = input.EntityID;

   /*  output.Depth = LinearizeDepth(input.Position.z, sceneInfo.nearPlane, sceneInfo.farPlane); */

    return output;
}
