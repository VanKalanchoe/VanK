#pragma once

//public includes visible to sandbox

//-----Public API--------
#include "VanK/Core/Application.h"
#include "VanK/core/Timestep.h"
#include "imgui.h"

//-------Rendering------
//#include "VanK/Renderer/Renderer.h"
#include "VanK/Renderer/Renderer2D.h"
#include "VanK/Scene/Scene.h"
#include "VanK/Scene/Components.h"
#include "VanK/Scene/Entity.h"
//-----------------------

//-----Loading Resources---
#include "VanK/Renderer/Shader.h"
#include "VanK/Renderer/Texture.h"
//-------------------------

//-------Utility-----------
#include "VanK/Renderer/OrthographicCameraController.h"
#include "VanK/Renderer/OrthographicCamera.h"
#include "VanK/Core/Input.h"
#include "VanK/Debug/Instrumentor.h"
//-------------------------

//--------Layer----------
#include "VanK/Core/Layer.h"
#include "VanK/ImGui/ImGuiLayer.h"
//-----------------------