#include <iostream>
#include <ostream>

#include "Vank/Core/VanK.h"
#include "Sandbox2D.h"
#include "VanK/Core/Log.h"

/*class ExampleLayer : public VanK::Layer
{
public:
    ExampleLayer() : Layer("Example"), m_CameraController(1280.0f / 720.0f, true)
    {
        //add custom create shader/texture 
        //simplify it like this   m_CheckerboardTexture = Hazel::Texture2D::Create("assets/textures/Checkerboard.png");

        // Load the images
        imageData = VanK::Texture::LoadImage("char (2).png", 4);
        if (imageData == nullptr)
        {
            SDL_Log("Could not load first image data!");
        }

        // Load the images
        imageData2 = VanK::Texture::LoadImage("char2 (1).png", 4);
        if (imageData2 == nullptr)
        {
            SDL_Log("Could not load second image data!");
        }

        // Load the images
        imageData3 = VanK::Texture::LoadImage("tanteemma.png", 4);
        if (imageData3 == nullptr)
        {
            SDL_Log("Could not load second image data!");
        }

        // Load the images
        imageData4 = VanK::Texture::LoadImage("tiles.png", 4);
        if (imageData4 == nullptr)
        {
            SDL_Log("Could not load second image data!");
        }
        // move this to texture.cpp maybe ??
        /*noImageGPU = VanK::renderer->createTexture(nullptr);
        imageDataGPU = VanK::renderer->createTexture(imageData);
        imageDataGPU2 = VanK::renderer->createTexture(imageData2);
        imageDataGPU3 = VanK::renderer->createTexture(imageData3);
        imageDataGPU4 = VanK::renderer->createTexture(imageData4);#1#
    }
    
    glm::vec4 color = {1.0f, 1.0f, 0.0f, 1.0f};
    void OnUpdate(VanK::Timestep ts) override
    {
        std::cout << "OnUpdate" << ts.GetMilliseconds() << std::endl;
        
        //Update
        m_CameraController.OnUpdate(ts);
        
        //std::cout << "sec: " << ts.GetSeconds() << " ms: " << ts.GetMilliseconds() << std::endl;
        
        //Render
        t += 1.0f * ts.GetSeconds();
        
        VanK::renderer->BeginScene(m_CameraController.GetCamera());

        VanK::renderer->setColor(0.212f, 0.075f, 0.541f, 1.0f);
        
        //VanK::renderer->DrawQuad({-0.5, -0.5, 1.0f}, {1.5f, 0.75f}, color);
        //VanK::renderer->DrawQuad({-0.5, 0.5}, imageDataGPU2);
        //VanK::renderer->DrawQuad({0.5, -0.5},45.0f * t, imageDataGPU3);
        //VanK::renderer->DrawQuad({0.5, -0.5},45.0f * t,color, imageData3);
        //VanK::renderer->DrawQuad({0.5, 0.5}, 45.0f * t , 1.0f, {1.0f * t, 1.0f * t, 1.0f * t}, imageData3);
        //VanK::renderer->DrawQuad({0.5, 0.5},   imageDataGPU4);
        
        float spacing = 0.11f;  // Adjust the spacing value to control distance between quads
        float quadSize = 1.0f;  // Size of each quad
        
        for (int y = 0; y < 200; y++)
        {
            for (int x = 0; x < 20; x++)
            {
                float quadX = x * (quadSize + spacing);  // Horizontal position with spacing
                float quadY = y * (quadSize + spacing);  // Vertical position with spacing
                VanK::renderer->DrawQuad({quadX,  quadY, 0.0f}, color);   
            }
        }
        
        VanK::renderer->EndScene();
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Settings");

       
        ImGui::Text("Renderer stats:");
      
        ImGui::ColorEdit4("Position", glm::value_ptr(color));
        ImGui::End();
    }

    void OnEvent(VanK::Event& e) override
    {
        m_CameraController.OnEvent(e);
    }

private:
    //------------camera-------------
    VanK::OrthographicCameraController m_CameraController;
    /*glm::vec3 cameraPosition;
    float cameraMoveSpeed = 1.0f;
    float cameraRotation = 0.0f;
    float cameraRotationSpeed = 1.0f;#1#
    //-------------------------------
    
    float t = 0;
    SDL_Surface* imageData;
    SDL_Surface* imageData2;
    SDL_Surface* imageData3;
    SDL_Surface* imageData4;
    SDL_GPUTexture* imageDataGPU;
    SDL_GPUTexture* imageDataGPU2;
    SDL_GPUTexture* imageDataGPU3;
    SDL_GPUTexture* imageDataGPU4;
    SDL_GPUTexture* noImageGPU;
};*/

class SandBox : public VanK::Application
{
public:
    SandBox(const VanK::ApplicationSpecification& specification) : Application(specification)
    {
        VK_INFO("SandBox::SandBox()");
        //PushLayer(new ExampleLayer());
        PushLayer(new Sandbox2D());
    }

    ~SandBox() override
    {
        VK_INFO("SandBox::~SandBox()");
    }
};

VanK::Application* VanK::CreateApplication(ApplicationCommandLineArgs args) //args
{
    ApplicationSpecification spec;
    spec.Name = "SandBox";
    spec.WorkingDirectory = "../VanK-Editor";
    spec.CommandLineArgs = args;
    
    return new SandBox(spec);
}