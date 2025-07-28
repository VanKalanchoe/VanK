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
    [[vk::location(LVWorldPosition)]] // Vulkan layout(location = 0)
    float4 WorldPosition : POSITION;  // Vertex position in model space
    [[vk::location(LVLocalPosition)]] // Vulkan layout(location = 2)
    float3 LocalPosition : TEXCOORD0;       // Texture coordinates
    [[vk::location(LVColorCircle)]]    // Vulkan layout(location = 1)
    float4 Color : COLOR;        // Vertex color
    [[vk::location(LVThickness)]] // Vulkan layout(location = 2)
    float Thickness : TEXCOORD1;       // Texture coordinates
    [[vk::location(LVFade)]] // Vulkan layout(location = 2)
    float Fade : TEXCOORD2;       // Texture coordinates
    [[vk::location(LVEntityIDCircle)]] 
    int EntityID : USER_DEFINED_SEMANTIC;
};

// Define the data that will be interpolated and passed from vertex to fragment shader
// Note: SV_Position is a special semantic in Slang/HLSL for the clip-space position
struct VertexOutput
{
    float4 WorldPosition : SV_Position; // Clip space position (required)
    float3 LocalPosition : TEXCOORD0;         // Interpolated texture coordinates
    float4 Color : TEXCOORD1;      // Interpolated vertex color
    float Thickness : TEXCOORD2;
    float Fade : TEXCOORD3;
    int EntityID : TEXCOORD4;
};

// Define the final output of the fragment shader
// Note: SV_TARGET specifies this is the color output to the render target
struct PixelOutput
{
    float4 Color : SV_TARGET0; // Final color output (RGBA)
    int EntityID : SV_TARGET1; // Final color output (RGBA)
    /* float Depth : SV_Depth; */
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
    VertexOutput output;
    output.WorldPosition = mul(sceneInfo.MatrixTransform, input.WorldPosition);
    output.WorldPosition.y *= -1.0f; // Flip to compensate for Vulkan's Y-down NDC
    output.LocalPosition = input.LocalPosition;
    output.Color = input.Color;
    output.Thickness = input.Thickness;
    output.Fade = input.Fade;
    output.EntityID = input.EntityID;
    return output;
}

//------------------------------------------------------------------------------
// Fragment/Pixel Shader
//------------------------------------------------------------------------------
/* 
float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0;
    return ((2.0 * near * far) / (far + near - z * (far - near))) / far;
} */

// Note: [shader("pixel")] is Slang's way of marking the fragment shader entry point
[shader("pixel")]
PixelOutput fragmentMain(VertexOutput input)
{
    PixelOutput output;

    // Calculate distance and fill circle with white
    float distance = 1.0 - length(input.LocalPosition);
    float circle = smoothstep(0.0, input.Fade, distance);
    circle *= smoothstep(input.Thickness + input.Fade, input.Thickness, distance);

    if(circle == 0)
        discard;

    // Set output color

    output.Color = input.Color;
    output.Color.a *= circle;

    output.EntityID = input.EntityID;

    /* output.Depth = LinearizeDepth(input.WorldPosition.z, sceneInfo.nearPlane, sceneInfo.farPlane);
 */
    return output;
}
