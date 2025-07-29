#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "VanK/Core/UUID.h"

#include "VanK/Scene/Scene.h"

#include "VanK/Scene/Entity.h"

#include "VanK/Core/log.h"

#include "mono/metadata/object.h"

#include <glm/gtx/string_cast.hpp>

#include <mono/metadata/reflection.h>

#include "VanK/Core/Input.h"

namespace VanK
{
    static std::unordered_map<MonoType*, std::function<bool(Entity)>> s_EntityHasComponentFuncs;
    
    #define VK_ADD_INTERNAL_CALL(Name) mono_add_internal_call("VanK.InternalCalls::" #Name, Name)
    
    static void NativeLog(MonoString* string, int parameter)
    {
        char* cStr = mono_string_to_utf8(string);
        std::string str(cStr);
        mono_free(cStr);
        std::cout << str << ", " << std::to_string(parameter) << std::endl;
    }

    static void NativeLog_Vector(glm::vec3* parameter, glm::vec3* outResult)
    {
        VK_CORE_WARN("Value: {0}", glm::to_string(*parameter));

        *outResult =  glm::normalize(*parameter);
    }

    static float NativeLog_VectorDot(glm::vec3* parameter)
    {
        VK_CORE_WARN("Value: {0}", glm::to_string(*parameter));

        return  glm::dot(*parameter, *parameter);
    }

    static void TransformComponent_GetTranslation(UUID entityID, glm::vec3* outTranslation)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        VK_CORE_ASSERT(scene, "Scene not available");
        Entity entity = scene->GetEntityByUUID(entityID);
        VK_CORE_ASSERT(entity, "Entity not available {}");
        
        *outTranslation = entity.GetComponent<TransformComponent>().Position;
    }

    static MonoObject* GetScriptInstance(UUID entityID)
    {
        return ScriptEngine::GetManagedInstance(entityID);
    }
    
    static bool Entity_HasComponent(UUID entityID, MonoReflectionType* componentType)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        VK_CORE_ASSERT(scene, "Scene not available");
        Entity entity = scene->GetEntityByUUID(entityID);
        VK_CORE_ASSERT(entity, "Entity not available {}");
        
        MonoType* managedType = mono_reflection_type_get_type(componentType);
        VK_CORE_ASSERT(s_EntityHasComponentFuncs.find(managedType) != s_EntityHasComponentFuncs.end(), "Component not registered")
        return s_EntityHasComponentFuncs.at(managedType)(entity);
    }

    static uint64_t Entity_FindEntityByName(MonoString* name)
    {
        char* namecCStr = mono_string_to_utf8(name);
        Scene* scene = ScriptEngine::GetSceneContext();
        VK_CORE_ASSERT(scene, "Scene not available");
        Entity entity = scene->FindEntityByName(namecCStr);
        mono_free(namecCStr);
        
        if (!entity)
            return 0;
        
       return entity.GetUUID();
    }

    static void TransformComponent_SetTranslation(UUID entityID, glm::vec3* translation)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        VK_CORE_ASSERT(scene, "Scene not available");
        Entity entity = scene->GetEntityByUUID(entityID);
        VK_CORE_ASSERT(entity, "Entity not available {}");
        
       entity.GetComponent<TransformComponent>().Position = *translation;
    }

    static void TransformComponent_ApplyLinearImpulse(UUID entityID, glm::vec2* impulse, glm::vec2* point, bool wake)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        VK_CORE_ASSERT(scene, "Scene not available");
        Entity entity = scene->GetEntityByUUID(entityID);
        VK_CORE_ASSERT(entity, "Entity not available {}");
        
        auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
        b2BodyId body = rb2d.RuntimeBody;
        b2Body_ApplyLinearImpulse(body, b2Vec2(impulse->x, impulse->y), b2Vec2(point->x, point->y), wake);
    }

    static void TransformComponent_ApplyLinearImpulseToCenter(UUID entityID, glm::vec2* impulse, bool wake)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        VK_CORE_ASSERT(scene, "Scene not available");
        Entity entity = scene->GetEntityByUUID(entityID);
        VK_CORE_ASSERT(entity, "Entity not available {}");
        
        auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
        b2BodyId body = rb2d.RuntimeBody;
        b2Body_ApplyLinearImpulseToCenter(body, b2Vec2(impulse->x, impulse->y),  wake);
    }

    static bool Input_IsKeyDown(SDL_Scancode scanCode)
    {
        return Input::IsKeyPressed(scanCode);
    }

    template<typename... Component>
    static void RegisterComponent()
    {
        ([]()
        {
            std::string_view typeName = typeid(Component).name();
            size_t pos = typeName.find_last_of(":");
            std::string_view structName = typeName.substr(pos + 1);
            std::string managedTypename = fmt::format("VanK.{}", structName);
            
            MonoType* managedType = mono_reflection_type_from_name(managedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
            if (!managedType)
            {
                VK_CORE_ERROR("Could not find component type {}", managedTypename);
                return;
            }
            s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); }; 
        }(), ...);
    }

    template<typename... Component>
    static void RegisterComponent(ComponentGroup<Component...>)
    {
        RegisterComponent<Component...>();
    }

    void ScriptGlue::RegisterComponents()
    {
        s_EntityHasComponentFuncs.clear();
        RegisterComponent(AllComponents{});
    }

    void ScriptGlue::RegisterFunctions()
    {
        VK_ADD_INTERNAL_CALL(NativeLog);
        VK_ADD_INTERNAL_CALL(NativeLog_Vector);
        VK_ADD_INTERNAL_CALL(NativeLog_VectorDot);
        
        VK_ADD_INTERNAL_CALL(GetScriptInstance);
        VK_ADD_INTERNAL_CALL(Entity_HasComponent);
        VK_ADD_INTERNAL_CALL(Entity_FindEntityByName);
        
        VK_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
        VK_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);

        VK_ADD_INTERNAL_CALL(TransformComponent_ApplyLinearImpulse);
        VK_ADD_INTERNAL_CALL(TransformComponent_ApplyLinearImpulseToCenter);
        
        VK_ADD_INTERNAL_CALL(Input_IsKeyDown);
    }
}
