using System.ComponentModel;

namespace VanK
{
    public abstract class Component
    {
        public Entity Entity { get; internal set; }
    }
    
    public class TransformComponent : Component
    {
        public Vector3 Translation
        {
            get
            {
                InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 translation);
                return translation;
            }
            
            set
            {
                InternalCalls.TransformComponent_SetTranslation(Entity.ID, ref value);
            }
        }
    }

    public class RigidBody2DComponent : Component
    {
        public void ApplyLinearImpulse(Vector2 impulse, Vector2 worldPosition, bool wake)
        {
            InternalCalls.TransformComponent_ApplyLinearImpulse(Entity.ID, ref impulse, ref worldPosition, ref wake);
        }
        public void ApplyLinearImpulse(Vector2 impulse, bool wake)
        {
            InternalCalls.TransformComponent_ApplyLinearImpulseToCenter(Entity.ID, ref impulse, ref wake);
        }
    }
}