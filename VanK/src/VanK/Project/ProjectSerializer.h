#pragma once

#include "Project.h"
#include "VanK/Core/core.h"

namespace VanK
{
    class ProjectSerializer
    {
    public:
        ProjectSerializer(Ref<Project> project);

        bool Serialize(const std::filesystem::path& filePath);
        bool Deserialize(const std::filesystem::path& filePath);
    private:
        Ref<Project> m_Project;
    };
}
