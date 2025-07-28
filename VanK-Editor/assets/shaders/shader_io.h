
#ifndef HOST_DEVICE_H
#define HOST_DEVICE_H

#ifdef __SLANG__
typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;
typealias mat4 = column_major float4x4;
#define STATIC_CONST static const
#else
#define STATIC_CONST const
#endif

// Layout constants
// Set 0
STATIC_CONST int LSetTextures  = 0;
STATIC_CONST int LBindTextures = 0;
// Set 1
STATIC_CONST int LSetScene      = 1;
STATIC_CONST int LBindSceneInfo = 0;
// Vertex layout
STATIC_CONST int LVPosition = 0;
STATIC_CONST int LVTexCoord = 1;
STATIC_CONST int LVColor = 3;
STATIC_CONST int LVEntityID = 4;
STATIC_CONST int LVTextureID = 5;

// Vertex Circle layout
STATIC_CONST int LVWorldPosition = 0;
STATIC_CONST int LVLocalPosition = 1;
STATIC_CONST int LVColorCircle = 3;
STATIC_CONST int LVThickness = 4;
STATIC_CONST int LVFade = 5;
STATIC_CONST int LVEntityIDCircle = 6;

// Vertex Line layout
STATIC_CONST int LVLinePosition = 0;
STATIC_CONST int LVLineColor = 2;
STATIC_CONST int LVLineEntityID = 3;

struct SceneInfo
{//camera stuff here because push constant not big enough
   mat4 MatrixTransform;
   float nearPlane;
   float farPlane;
};

struct PushConstant
{
  vec3 color;
};

struct PushConstantCompute
{
  int numVertex;
};

struct Vertex
{
  vec4 Position;
  vec2 Texcoord;
  vec2 pad1;
  vec4 Color;
  // Editor-only
  int EntityID;
  int TextureID;
  vec2 pad3;
};

struct CircleVertex
{
  vec4 WorldPosition;
  vec3 LocalPosition;
  float pad0;
  vec4 Color;
  float Thickness;
  float Fade;

  // Editor-only
  int EntityID;
  float pad1;
};

struct LineVertex
{
  vec3 Position;
  float pad;
  vec4 Color;

  // Editor-only
  int EntityID;
  vec3 pad1;
};


struct CircleInstance
{
  //float x, y, z;
  vec3 WorldPosition;
  float _pad0;
  vec3 LocalPosition;
  float _pad1;
  vec3 Rotation;
  float _pad2;
  //float w, h, padding_a, padding_b;
  vec2 Size;
  vec2 pad3;
  vec3 Scale;
  float pad4;
  vec4 Color;
  //float r, g, b, a;
  float Thickness;
  float Fade;
        
  // Editor-only
  int EntityID;
  float pad5;
};


struct QuadInstance
{
  //float x, y, z;
  vec3 Position;
  float _pad0;
  vec3 Rotation;
  float _pad1;
  //float w, h, padding_a, padding_b;
  vec2 Size;
  vec2 _pad2;     // 8 bytes padding
  vec3 Scale;
  int TextureID;
  vec4 Color;
  //float r, g, b, a;
  vec2 tilePosition;
  vec2 tileSize;
  vec2 tileMultiplier;
  vec2 atlasSize;
  vec2 _pad4;    
  float TilingFactor;
  
  // Editor-only
  int EntityID;
};

#endif  // HOST_DEVICE_H
