#include <iostream>
#include <ostream>

#include "Vank/Core/VanK.h"
#include "EditorLayer.h"
#include "VanK/Core/Log.h"

namespace VanK
{
    class VanKEditor : public Application
    {
    public:
        VanKEditor(const ApplicationSpecification& spec) : Application(spec)
        {
            VK_INFO("EditorLayer::EditorLayer()");
            PushLayer(new EditorLayer());
        }

        ~VanKEditor() override //delete this atleast cherno did that 
        {
            VK_INFO("EditorLayer::~EditorLayer()");
        }
    };

    Application* CreateApplication(ApplicationCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.Name = "VanK-Editor";
        //spec.WorkingDirectory = "../VanK-Editor";
        spec.CommandLineArgs = args;
        
        return new VanKEditor(spec);
    } 
}
