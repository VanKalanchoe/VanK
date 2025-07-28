#pragma once
#include <expected>
#include <string>
#include <unordered_map>

#include "VanK/Renderer/Shader.h"
#include <glm/glm.hpp>
#include "volk.h"
namespace VanK
{
    struct ShaderModuleInfo;
    struct ShaderStageInfo;

    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const std::string& filePath);
        VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~VulkanShader() override;

        virtual void Bind() const override;
        virtual void Unbind() const override;

        VkShaderModule GetShaderModule(VkShaderStageFlagBits stage) const;
        std::string GetShaderEntryName(VkShaderStageFlagBits stage) const;
        virtual const std::string& GetName() const override { return m_Name; };
        const std::string& GetFilePath() const override { return m_FilePath; }

        void UpdateUniformInt(const std::string& name, int value);

        void UpdateUniformFloat(const std::string& name, float value);
        void UpdateUniformFloat2(const std::string& name, const glm::vec2& value);
        void UpdateUniformFloat3(const std::string& name, const glm::vec3& value);
        void UpdateUniformFloat4(const std::string& name, const glm::mat4& value);
    
        void UpdateUniformMat3(const std::string& name, const glm::mat3& matrix);
        void UpdateUniformMat4(const std::string& name, const glm::mat4& matrix);

    private:
        std::expected<std::unordered_map<VkShaderStageFlagBits, ShaderStageInfo>, std::string> compileSlang();
        std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> PreProcess(
            const std::string& source);
        void Compile(const std::unordered_map<VkShaderStageFlagBits, ShaderStageInfo>& shaderSources);
    private:
        uint32_t m_RendererID;
        std::string m_Name;
        std::string m_FilePath;
        std::unordered_map<VkShaderStageFlagBits, ShaderModuleInfo> m_ShaderModules;
    };
}