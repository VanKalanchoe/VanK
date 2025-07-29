#pragma once

#include <filesystem>

#include "VanK/Core/Buffer.h"

namespace VanK
{
    class FileSystem
    {
    public:
        static Buffer ReadFileBinary(const std::filesystem::path& filepath);
    };
}
