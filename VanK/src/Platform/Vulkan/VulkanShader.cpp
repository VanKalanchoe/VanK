#include "VulkanShader.h"

#include "VanK/Renderer/Shader.h"

#include "VulkanRendererAPI.h"

#include "slang.h"
#include "slang-com-ptr.h"
#include "slang-com-helper.h"
#include "VanK/Utils.h"
#include <print>
#include <expected>
#include <utility>

namespace VanK
{
    void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
    {
        if (diagnosticsBlob != nullptr)
        {
            std::cout << static_cast<const char*>(diagnosticsBlob->getBufferPointer()) << std::endl;
        }
    }

    struct ShaderStageInfo {
        std::string entryPointName;
        std::vector<uint32_t> spirvCode;
    };

    std::expected<std::unordered_map<VkShaderStageFlagBits, ShaderStageInfo>, std::string> VulkanShader::compileSlang()
    {
        bool forceCompile = false;
        // All EntryPoints possible
        std::vector<std::string> EntryPoints
        {
            "vertexMain",
            "fragmentMain",
            "main", // maybe i can call this computemain ? will see
        };

        std::unordered_map<VkShaderStageFlagBits, ShaderStageInfo> spirvPerStage;

        std::string cachePath = Utils::GetCachePath();
        
        std::string fileHashName = m_Name;
        XXH128_hash_t currentHash = Utils::calcul_hash_streaming(m_FilePath);
        XXH128_hash_t cachedHash{};
        std::string hashFile = cachePath + fileHashName + ".hash";
        bool hashMatches = Utils::loadHashFromFile(hashFile, cachedHash) && (cachedHash.low64 == currentHash.low64 && cachedHash.high64 == currentHash.high64);
        std::cout << "[Hash] Current: " << std::hex << currentHash.high64 << currentHash.low64 << '\n';
        std::cout << "[Hash] Cached : " << std::hex << cachedHash.high64 << cachedHash.low64 << '\n';

        if (hashMatches && !forceCompile)
        {
            for (std::string entryPoint : EntryPoints)
            {
                // Map known entry points to Vulkan shader stages
                VkShaderStageFlagBits stage;
                if (entryPoint == "vertexMain")         stage = VK_SHADER_STAGE_VERTEX_BIT;
                else if (entryPoint == "fragmentMain")  stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                else if (entryPoint == "main")          stage = VK_SHADER_STAGE_COMPUTE_BIT;
                else continue;
                
                std::string fileName = m_Name + "." + entryPoint;
                std::string fullPath = cachePath + fileName + ".spv";
                // Check existence first
                if (!std::filesystem::exists(fullPath))
                {
                    std::cout << "[Hash] File '" << fullPath << "' doesn't exist." << std::endl;
                    continue;  
                }
                
                auto data = Utils::LoadSpvFromPath(fullPath);
                if (data.empty()) continue;
                spirvPerStage[stage] = ShaderStageInfo{entryPoint, std::move(data)};
            }
            return spirvPerStage;
        }
        
        Slang::ComPtr<slang::IGlobalSession> globalSession;
        if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())))
        {
            return std::unexpected<std::string>("Slang Failed to create Global Session");
        }
        
        slang::SessionDesc sessionDesc = {};
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = globalSession->findProfile("spirv_1_5");
        
        // can enable multipletargets at once so maybe move out of vulkanshader class ?
        
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;
        
        std::array<slang::CompilerOptionEntry, 2> options = 
        {
            {
                slang::CompilerOptionName::EmitSpirvDirectly,
                {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr},
            
                slang::CompilerOptionName::VulkanUseEntryPointName,
                { slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr },
            }
        };
        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.compilerOptionEntryCount = options.size();
        
        Slang::ComPtr<slang::ISession> session;
        if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
        {
            return std::unexpected<std::string>("Slang Failed to create Session");
        }

        // Creates slangModule out of fileSource
        Slang::ComPtr<slang::IModule> slangModule;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            const char* moduleName = m_Name.c_str();
            const char* modulePath = m_FilePath.c_str();
            std::string sourceCode = Utils::LoadFileFromPath(modulePath);
            slangModule = session->loadModuleFromSourceString(moduleName, modulePath, sourceCode.c_str(),
                                                              diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            if (!slangModule)
            {
                return std::unexpected<std::string>("Slang Failed to load module");
            }
        }
        
        std::vector<Slang::ComPtr<slang::IEntryPoint>> foundEntryPoints;
        std::vector<slang::IComponentType*> componentTypes;
        componentTypes.push_back(slangModule);

        for (int i = 0; i < EntryPoints.size(); i++)
        {
            Slang::ComPtr<slang::IEntryPoint> entryPoint;
            {
                Slang::ComPtr<slang::IBlob> diagnosticsBlob;
                slangModule->findEntryPointByName(EntryPoints.at(i).c_str(), entryPoint.writeRef());
                if (!entryPoint)
                {
                    continue;
                }
                foundEntryPoints.push_back(entryPoint);
                componentTypes.push_back(entryPoint);
            }
        }

        /*std::array<slang::IComponentType*, 2> componentTypes =
        {
            slangModule,
            entryPoint
        };*/

        Slang::ComPtr<slang::IComponentType> composedProgram;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            SlangResult result = session->createCompositeComponentType(
                componentTypes.data(),
                componentTypes.size(),
                composedProgram.writeRef(),
                diagnosticsBlob.writeRef());
            diagnoseIfNeeded(diagnosticsBlob);
            if (SLANG_FAILED(result))
                return std::unexpected<std::string>("Slang operation failed with code: " + std::to_string(result));
        }

        // Links directly to slangModule to skip finding All EntryPoints by Slang::ComPtr<slang::IComponentType> composedProgram;
        Slang::ComPtr<slang::IComponentType> linkedProgram;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            SlangResult result = composedProgram->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            if (SLANG_FAILED(result))
                return std::unexpected<std::string>("Slang operation failed with code: " + std::to_string(result));
        }

        // Iterate trough all EntryPoints from spirvCode to map to what vkShaderModule needs each entry byitself
        slang::ProgramLayout* programLayout = linkedProgram->getLayout();

        SlangUInt entryPointCount = programLayout->getEntryPointCount();

        for (int i = 0; std::cmp_less(i, entryPointCount); ++i)
        {
            slang::EntryPointReflection* entryLayout = programLayout->getEntryPointByIndex(i);

            // Get Name
            const char* name = entryLayout->getName();

            // Get the shader stage
            SlangStage slangStage = entryLayout->getStage();

            // Map Slang stage to Vulkan stage
            VkShaderStageFlagBits stage;
            switch (slangStage)
            {
            case SLANG_STAGE_VERTEX: stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case SLANG_STAGE_FRAGMENT: stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case SLANG_STAGE_COMPUTE: stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default: std::cerr << "Unsupported shader stage for entry point: " << name << "\n";
                continue;;
            }
 
            // Get SPIR-V code for this entry point
            Slang::ComPtr<slang::IBlob> spirvCode;
            {
                Slang::ComPtr<slang::IBlob> diagnosticBlob;
                SlangResult result = linkedProgram->getEntryPointCode
                (
                    i, // entryPointIndex
                    0, // targetIndex
                    spirvCode.writeRef(),
                    diagnosticBlob.writeRef()
                );
                diagnoseIfNeeded(diagnosticBlob);
                if (SLANG_FAILED(result))
                    return std::unexpected<std::string>("Slang operation failed with code: " + std::to_string(result));
            }

            //reflection
            {
                //type layout size variable layout offset reflection
                slang::TypeLayoutReflection* typeLayout = entryLayout->getTypeLayout();
                //slang::VariableLayoutReflection* variableLayout = entryLayout->getVarLayout();
                //category descriptortableslot vk::binding binding, set bindingindex getoffset, setindex = getbindingspace
                //vk::binding is not needed in slang wtf mit parameterblocks möglich

                //descriptortype slang::BindingType
                
                //subelementregisterspace descriptorset indices
                int bindingRangeCount = typeLayout->getBindingRangeCount();
                int parameterCount = programLayout->getParameterCount();
                for (int j = 0; j < parameterCount; ++j)
                {
                    slang::VariableLayoutReflection* variableLayout = programLayout->getParameterByIndex(j);
                    auto layoutUnit = slang::ParameterCategory::DescriptorTableSlot;
                    auto varTypeLayout = variableLayout->getTypeLayout();
                    auto bindingCount = varTypeLayout->getBindingRangeCount();

                    for (SlangUInt k = 0; k < bindingCount; ++k)
                    {
                        auto bindingType = varTypeLayout->getBindingRangeType(k);
                        std::string bindTypeName = "";
                        switch (bindingType)
                        {
                        case slang::BindingType::Sampler:
                            bindTypeName = "Sampler";
                            break;
                        case slang::BindingType::Texture:
                            bindTypeName = "Texture";
                            break;
                        case slang::BindingType::ConstantBuffer:
                            bindTypeName = "ConstantBuffer";
                            break;
                        case slang::BindingType::ParameterBlock:
                            bindTypeName = "ParameterBlock";
                            break;
                        case slang::BindingType::TypedBuffer:
                            bindTypeName = "TypedBuffer";
                            break;
                        case slang::BindingType::RawBuffer:
                            bindTypeName = "RawBuffer";
                            break;
                        case slang::BindingType::CombinedTextureSampler:
                            bindTypeName = "CombinedTextureSampler";
                            break;
                        case slang::BindingType::InputRenderTarget:
                            bindTypeName = "InputRenderTarget";
                            break;
                        case slang::BindingType::InlineUniformData:
                            bindTypeName = "InlineUniformData";
                            break;
                        case slang::BindingType::RayTracingAccelerationStructure:
                            bindTypeName = "RayTracingAccelerationStructure";
                            break;
                        case slang::BindingType::VaryingInput:
                            bindTypeName = "VaryingInput";
                            break;
                        case slang::BindingType::VaryingOutput:
                            bindTypeName = "VaryingOutput";
                            break;
                        case slang::BindingType::ExistentialValue:
                            bindTypeName = "ExistentialValue";
                            break;
                        case slang::BindingType::PushConstant:
                            bindTypeName = "PushConstant";
                            break;
                        case slang::BindingType::MutableFlag:
                            bindTypeName = "MutableFlag";
                            break;
                        case slang::BindingType::MutableTexture:
                            bindTypeName = "MutableTexture";
                            break;
                        case slang::BindingType::MutableTypedBuffer:
                            bindTypeName = "MutableTypedBuffer";
                            break;
                        case slang::BindingType::MutableRawBuffer:
                            bindTypeName = "MutableRawBuffer";
                            break;
                        case slang::BindingType::BaseMask:
                            bindTypeName = "BaseMask";
                            break;
                        case slang::BindingType::ExtMask:
                            bindTypeName = "ExtMask";
                            break;
                        default:
                            bindTypeName = "Unknown";
                        }
                        
                        auto bindingName = varTypeLayout->getName(); // Get name
                        //auto set = variableLayout->getBindingSpace(layoutUnit); beide möglich ?
                        auto set = variableLayout->getBindingSpace(layoutUnit);
                        //auto binding = variableLayout->getBindingIndex();//beide möglich ?
                        auto binding = variableLayout->getOffset(layoutUnit);
                        
                        std::cout << "Name: " << bindingName << std::endl;
                        std::cout << "Type: " << bindTypeName << std::endl;
                        std::cout << "Set: " << set << std::endl;
                        std::cout << "Binding: " << binding << std::endl;
                        std::cout << "Binding Count: " << varTypeLayout->getBindingRangeBindingCount(k) << std::endl << std::endl;
                    }
                }
            }
            
            // converting to usable byte code and saving cache/hash
            std::vector<uint32_t> spirvCodeToUint32;
            {
                auto byteSize = spirvCode->getBufferSize();
                auto ptr = static_cast<const uint32_t*>(spirvCode->getBufferPointer());
                spirvCodeToUint32.assign(ptr, ptr + byteSize / sizeof(uint32_t));
            }
            
            std::string fileName = m_Name + "." + name;
            std::string fullPath = cachePath + fileName + ".spv";
            
            Utils::SaveToFile(fullPath.c_str(), spirvCodeToUint32.data(), spirvCodeToUint32.size() * sizeof(uint32_t));
            
            Utils::saveHashToFile(hashFile, currentHash);
            
            spirvPerStage[stage] = ShaderStageInfo{ std::string(name), std::move(spirvCodeToUint32)};
        }

        return spirvPerStage;
    }

    std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> VulkanShader::PreProcess(
        const std::string& filepath)
    {
        std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderSources;

#ifdef USE_SLANG
        /*
         retrieve source code and return to compile function
         1. compare hash file if its the same return cached file else compile and save file in cache spv -------------later
         2. compile file with slang if multiple entrypoints
         3. cachename should be filname+entrypoint and store it to use later for getting the correct cache spv-------------later
         4. save modules to later get/retrieve them in pipeline creation
 
         slang compile
         1. check all entrypoints possible if it exists in file if so compile it and store shader byte code somewhere
         2. every shaderlibrary load function should do that and with filename+entrypoint no overwriting possible nice
         3. reflection after compile works on multiple shadertype for example 2 computes right now it crashes because the ----
         compute shader has different bindings then first -------------later
         */

        /*compileSlang(); //give entrypoints to this*/
#else
        auto spirvShaders = loadOrCompileShaderGLSL("shaderModule", filepath);
        for (const auto& [type, spirv] : spirvShaders)
        {
            VkShaderStageFlagBits stage = ShaderTypeFromString(type);
            shaderSources[stage] = spirv;
        }
#endif

        return shaderSources;
    }
    
    struct ShaderModuleInfo {
        VkShaderModule module;
        std::string entryPointName;
    };
    
    void VulkanShader::Compile(const std::unordered_map<VkShaderStageFlagBits, ShaderStageInfo>& shaderSources)
    {
        std::cout << "Compile called with " << shaderSources.size() << " shader stages\n";
        auto device = VulkanRendererAPI::Get().GetContext().getDevice();
        for (const auto& [stage, spirv] : shaderSources)
        {
            std::cout << "entrypouint " << spirv.entryPointName << " module " << spirv.spirvCode.data() << std::endl;
            VkShaderModule shaderModule = utils::createShaderModule(
                device, std::span<const uint32_t>(spirv.spirvCode.data(), spirv.spirvCode.size()));
            DBG_VK_NAME(shaderModule);
            m_ShaderModules[stage] = ShaderModuleInfo{ shaderModule, spirv.entryPointName };
        }
        //maybe in the future store it in here and then only that specific shader has the correct stuff it overwrites grapgics because compute is last fixed inside sershadermodule
    }

    VulkanShader::VulkanShader(const std::string& fileName)
    {
        const std::vector<std::string> searchPaths = {
            ".", "shaders", "../shaders", "../../shaders", "../../../VanK-Editor/assets/shaders"
        };

        std::string shaderFile = utils::findFile(fileName, searchPaths);
        std::cout << "Shader file: " << shaderFile << '\n';

        m_FilePath = shaderFile;

        //get from filepath the name of the file maybe sdl3 can do that autoamticly Extract name from filepath
        auto lastSlash = shaderFile.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = shaderFile.rfind('.');
        auto count = lastDot == std::string::npos ? shaderFile.size() - lastSlash : lastDot - lastSlash;
        m_Name = shaderFile.substr(lastSlash, count);
        std::cout << "Shader name: " << m_Name << '\n';

        //error handling here
        Compile(compileSlang().value());
    }

    VulkanShader::VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_Name(name)
    {
        /*for (const auto& [stage, module] : m_ShaderModules)
        {
            std::cout << "Shader " << m_Name << " stage " << stage << " module " << module << '\n';
        }*/
        /*VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    
        std::unordered_map<VkShaderStageFlagBits, std::string> sources;
        sources[ShaderTypeFromString("vertex")] = vertexSrc;
        sources[ShaderTypeFromString("fragment")] = fragmentSrc;
        Compile(sources);*/
    }

    VulkanShader::~VulkanShader()
    {
        std::cout << "Shader destroyed: " << m_Name << '\n';
        VkDevice device = VulkanRendererAPI::Get().GetContext().getDevice();

        for (auto& [stage, module] : m_ShaderModules)
        {
            if (module.module != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(device, module.module, nullptr);
            }
        }

        m_ShaderModules.clear();
    }

    VkShaderModule VulkanShader::GetShaderModule(VkShaderStageFlagBits stage) const
    {
        auto it = m_ShaderModules.find(stage);
        if (it != m_ShaderModules.end())
            return it->second.module;

        return VK_NULL_HANDLE;
    }

    std::string VulkanShader::GetShaderEntryName(VkShaderStageFlagBits stage) const
    {
        auto it = m_ShaderModules.find(stage);
        if (it != m_ShaderModules.end())
            return it->second.entryPointName;

        return "";
    }

    void VulkanShader::Bind() const
    {
    }

    void VulkanShader::Unbind() const
    {
    }

    void VulkanShader::UpdateUniformInt(const std::string& name, int value)
    {
    }

    void VulkanShader::UpdateUniformFloat(const std::string& name, float value)
    {
    }

    void VulkanShader::UpdateUniformFloat2(const std::string& name, const glm::vec2& value)
    {
    }

    void VulkanShader::UpdateUniformFloat3(const std::string& name, const glm::vec3& value)
    {
    }

    void VulkanShader::UpdateUniformFloat4(const std::string& name, const glm::mat4& value)
    {
    }

    void VulkanShader::UpdateUniformMat3(const std::string& name, const glm::mat3& matrix)
    {
    }

    void VulkanShader::UpdateUniformMat4(const std::string& name, const glm::mat4& matrix)
    {
    }
}