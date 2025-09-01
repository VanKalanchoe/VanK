#include "Sandbox2D.h"

#include "VanK/Asset/AssetManager.h"
#include "Vank/Core/VanK.h"
#include "VanK/Renderer/Renderer.h"
#include "VanK/Scripting/ScriptEngine.h"

Sandbox2D::Sandbox2D() : Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::OnAttach()
{
    VK_PROFILE_FUNCTION();

    std::filesystem::current_path("E:/dev/VanK/build/win/Facerun");
    // Load a project first
    if (VanK::Project::Load("E:/dev/VanK/build/win/Facerun/Facerun.vproj"))
    {
        VanK::ScriptEngine::Init();
        // Get the start scene from the project
        VanK::AssetHandle startScene = VanK::Project::GetActive()->GetConfig().StartScene;
        if (startScene)
        {
            // Load the scene using AssetManager
            m_RuntimeScene = VanK::AssetManager::GetAsset<VanK::Scene>(startScene);
        }
        else
        {
            // Fallback to empty scene
            m_RuntimeScene = VanK::CreateRef<VanK::Scene>();
        }
    }
    else
    {
        // Fallback to empty scene
        m_RuntimeScene = VanK::CreateRef<VanK::Scene>();
    }
    
    m_EditorCamera = VanK::EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);

    m_RuntimeScene->OnRuntimeStart();
}

void Sandbox2D::OnDetach()
{
    VK_PROFILE_FUNCTION();

    m_RuntimeScene->OnRuntimeStop();
}
VanK::Extent2D m_ViewportSizes
{
    800,600
};
void Sandbox2D::OnUpdate(VanK::Timestep ts)
{
    VanK::Renderer2D::ResetStats();
    glm::vec2 m_ViewportSize = { m_ViewportSizes.width, m_ViewportSizes.height };
    m_RuntimeScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
    if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f /*&& (Renderer2D::getSceneSize().x != m_ViewportSize.x ||
               Renderer2D::getSceneSize().y != m_ViewportSize.y)*/)
    {
      
        VanK::RenderCommand::OnViewportSizeChange({(uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y});
        VanK::Renderer::SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
  
        m_CameraController.OnResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y); //fix this or quad looks odd
    }
    
    m_CameraController.OnUpdate(ts);
    m_EditorCamera.OnUpdate(ts);
    
    /*VanK::Renderer::BeginScene(m_EditorCamera);
    
    VanK::Renderer2D::DrawQuad({-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f,0.0f,1.0f,1.0f });

    VanK::Renderer::EndScene();*/
    
    m_RuntimeScene->OnUpdateEditor(ts, m_EditorCamera);
}

void Sandbox2D::OnImGuiRender()
{
    VK_PROFILE_FUNCTION();
    // Settings window
    ImGui::Begin("Settings");

    auto stats = VanK::Renderer2D::GetStats();
    ImGui::Text("Renderer stats:");
    ImGui::Text("Draw Calls: %d", stats.DrawCalls);
    ImGui::Text("Quads : %d", stats.QuadCount);
    ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
    ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

    ImGui::ColorEdit4("Position", glm::value_ptr(squareColor));

    ImGui::End();  // End Settings Window
}

void Sandbox2D::OnEvent(VanK::Event& e)
{
    m_CameraController.OnEvent(e);
    
    m_EditorCamera.OnEvent(e);
}