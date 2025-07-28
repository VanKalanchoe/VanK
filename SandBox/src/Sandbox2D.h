#pragma once

#include "VanK/Core/VanK.h"

class Sandbox2D : public VanK::Layer
{
public:
    Sandbox2D();
    virtual ~Sandbox2D() = default;
    
    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(VanK::Timestep ts) override;
    virtual void OnImGuiRender() override;
    void OnEvent(VanK::Event& e) override;
    
private:
    glm::vec4 squareColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    VanK::OrthographicCameraController m_CameraController;
};
