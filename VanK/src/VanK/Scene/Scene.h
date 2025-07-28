#pragma once

#include "Vendor/Entt/include/entt.hpp"
#include "box2d/id.h"

#include "VanK/Core/Timestep.h"
#include "VanK/Renderer/EditorCamera.h"

#include "VanK/Core/UUID.h"

class b2world;

namespace VanK
{
    class Entity;
    
    class Scene
    {
    public:
        Scene();
        ~Scene();

        static Ref<Scene> Copy(Ref<Scene> other);

        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
        void DestroyEntity(Entity entity);

        void OnRuntimeStart();
        void OnRuntimeStop();
        
        void OnSimulationStart();
        void OnSimulationStop();
        
        void OnUpdateRuntime(Timestep ts);
        void OnUpdateSimulation(Timestep ts, EditorCamera& camera);
        void OnUpdateEditor(Timestep ts, EditorCamera& camera);
        void OnViewportResize(uint32_t width, uint32_t height);

        void DuplicateEntity(Entity entity);

        Entity GetEntityByUUID(UUID uuid);

        Entity GetPrimaryCameraEntity();

        bool IsRunning() const { return m_IsRunning; }

        template<typename... Components>
        auto GetAllEntitiesWith()
        {
            return m_Registry.view<Components...>();
        }
    private:
        template<typename T>
        void OnComponentAdded(Entity entity, T& component);

        void OnPhysics2DStart();
        void OnPhysics2DStop();

        void RenderScene(EditorCamera& camera);
    private:
        entt::registry m_Registry;
        uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
        float nearPlane, farPlane;

        b2WorldId m_PhysicsWorldID;
        bool m_IsRunning = false;

        std::unordered_map<UUID, entt::entity> m_EntityMap;

        friend class Entity;
        friend class SceneSerializer;
        friend class SceneHierarchyPanel;
    };
}
