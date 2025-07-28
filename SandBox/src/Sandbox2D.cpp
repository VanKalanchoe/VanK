#include "Sandbox2D.h"
#include "Vank/Core/VanK.h"

Sandbox2D::Sandbox2D() : Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::OnAttach()
{
    VK_PROFILE_FUNCTION();
    
    /*VanK::Texture::CreateTexture2D("Tiles.png", 4);
    VanK::Texture::CreateTexture2D("ChernoLogo.png", 4);*/
    // VanK::Texture::CreateTexture2D("Checkerboard.png", 4);
}

void Sandbox2D::OnDetach()
{
    VK_PROFILE_FUNCTION();
}

float t;

/*
void Sandbox2D::OnUpdate(VanK::Timestep ts)
{
    VanK::Renderer::ResetStats();
  
    m_CameraController.OnUpdate(ts);
    
    t += 1.0f * ts.GetSeconds();
    
    VanK::Renderer::SetColor({0.212f, 0.075f, 0.541f, 1.0f});
    
    VanK::Renderer::BeginScene(m_CameraController.GetCamera());
    
    //VanK::Renderer::DrawQuad({-0.5f, -0.5f}, {0.8f, 0.8f},{1.0f, 1.0f}, 0.0f, {1.0f,0.0f,1.0f,1.0f});
    VanK::Renderer::DrawQuad({0.5f, 0.5f}, {0.5f, 0.75f},{1.0f, 1.0f}, glm::radians(45.0f * t), {1.0f,1.0f,1.0f,1.0f});
    VanK::Renderer::DrawQuad({-0.5f, -0.5f}, {0.5f, 0.5f},{1.0f, 1.0f}, 0.0f, "Checkerboard.png", 10.0f, {1.0f,1.0f,1.0f,1.0f});
    VanK::Renderer::DrawQuad({-0.5f, 0.5f}, {0.5f, 0.50f},{1.0f, 1.0f}, glm::radians(45.0f * t), "char2.png", 1.0f, {1.0f,1.0f,1.0f,1.0f});
    
    VanK::Renderer::EndScene();

    /*VanK::Renderer::BeginScene(m_CameraController.GetCamera());
    for (float y = -5.0f; y < 10.0f; y+=0.5f)
    {
        for (float x = -5.0f; x < 10.0f; x+=0.5f)
        {
            glm::vec4 color = {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 1.0f};
            VanK::Renderer::DrawQuad({x, y}, {0.45f, 0.45f},{1.0f,1.0f} ,glm::radians(45.0f * t), "ChernoLogo.png", color); 
        }
    }
    VanK::Renderer::EndScene();#1#
}
*/
//you can use shadercross to build shaders at runtime
//Renderer2d
void Sandbox2D::OnUpdate(VanK::Timestep ts)
{
    VK_PROFILE_FUNCTION();
    
    /*VanK::Renderer2D::ResetStats();*/
    
    //Update
  
    m_CameraController.OnUpdate(ts);   
    
    // Render
    /*{
        VK_PROFILE_SCOPE("Renderer Prep");
        t += 1.0f * ts.GetSeconds();
        VanK::Renderer2D::SetColor({0.212f, 0.075f, 0.541f, 1.0f});
    }*/
    
    {
        VK_PROFILE_SCOPE("Renderer Draw");
        /*VanK::Renderer2D::BeginScene(m_CameraController.GetCamera(), "Tiles.png");
    for (float y = -5.0f; y < 5.0f; y+=0.5f)
    {
        for (float x = -5.0f; x < 5.0f; x+=0.5f)
        {
           // glm::vec4 color = {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 1.0f};
            VanK::Renderer2D::DrawQuad({x, y}, {0.45f, 0.45f},{1.0f,1.0f} ,glm::radians(45.0f * t), squareColor); 
        }
    }
    VanK::Renderer2D::DrawQuad({0.5f, -1.0f}, {0.5f, 0.75f},{1.0f, 1.0f}, glm::radians(45.0f * t), {1.0f,1.0f,1.0f,1.0f});
    VanK::Renderer2D::EndScene();*/
    //need z-ordering -> depth enable currently order is last one is above
    /*VanK::Renderer2D::BeginScene(m_CameraController.GetCamera(), "Tiles.png");
    VanK::Renderer2D::DrawQuad({-0.5f, -0.5f, 0.2f}, {1.0f, 1.0f},{1.0f, 1.0f}, glm::radians(0.0f * t), {1.0f,1.0f,1.0f,1.0f}, {8.0f, 20.0f}, {16.0f, 16.0f}, {1.0f, 1.0f}, {400.0f, 400.0f});
    VanK::Renderer2D::DrawQuad({-0.5f, 1.5f, 1.2f}, {2.0f, 3.0f},{1.0f, 1.0f}, glm::radians(0.0f * t), {1.0f,1.0f,1.0f,1.0f}, {2.0f, 15.0f}, {16.0f, 16.0f}, {2.0f, 3.0f}, {400.0f, 400.0f});
    VanK::Renderer2D::DrawQuad({-1.9f, -0.5f, 1.0f}, {3.0f, 3.0f},{1.0f, 1.0f}, glm::radians(0.0f), {1.0f,1.0f,1.0f,1.0f},{0.0f, 3.0f}, {16.0f, 16.0f}, {3.0f, 3.0f}, {400.0f, 400.0f});
    //VanK::Renderer2D::DrawQuad({-0.5f, -0.5f}, {0.5f, 0.50f},{1.0f, 1.0f}, glm::radians(45.0f * t), "ChernoLogo.png", 1.0f,{1.0f,1.0f,1.0f,1.0f});
   // VanK::Renderer2D::DrawQuad({-0.5f, 0.5f}, {0.5f, 0.50f},{1.0f, 1.0f}, glm::radians(45.0f * t),"whiteSurface", 1.0f,{1.0f,1.0f,1.0f,1.0f});
    VanK::Renderer2D::EndScene();*/

    /*VanK::Renderer2D::BeginScene(m_CameraController.GetCamera());
    //VanK::Renderer2D::DrawQuad({0.5f, -1.0f, 0.1f}, {1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    //VanK::Renderer2D::DrawQuad({0.3f, -1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, {1.0f, 0.0f, 1.0f, 1.0f});
    VanK::Renderer2D::EndScene();*/
    //init camaera only once change this
    VanK::Renderer2D::BeginScene(m_CameraController.GetCamera());
    VanK::Renderer2D::DrawQuad({2.5f, -2.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    VanK::Renderer2D::DrawQuad({2.3f, -2.0f, 0.1f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f});
    //VanK::Renderer2D::DrawQuad({0.0f, 0.0f, 0.1f}, {1.0f, 1.0f},{1.0f, 1.0f}, glm::radians(0.0f * t), {1.0f,1.0f,1.0f,1.0f}, {8.0f, 20.0f}, {16.0f, 16.0f}, {1.0f, 1.0f}, {400.0f, 400.0f});
    //VanK::Renderer2D::DrawQuad({0.0f, 0.0f, 0.0f}, {2.0f, 3.0f},{1.0f, 1.0f}, glm::radians(0.0f * t), {1.0f,1.0f,1.0f,1.0f}, {2.0f, 15.0f}, {16.0f, 16.0f}, {2.0f, 3.0f}, {400.0f, 400.0f});
    VanK::Renderer2D::EndScene();
    }
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
}