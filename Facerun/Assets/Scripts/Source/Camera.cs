using System;

using VanK;

namespace Sandbox
{
    public class Camera : Entity
    {
        public Entity otherEntity;
        
        public float DistanceFromPlayer = 5.0f;
        
        private float distanceFromPlayer = 0.0f;

        private Entity m_Player;
        
        void OnCreate()
        {
            m_Player = FindEntityByName("Player");
        }

        private float Lerp(float a, float b, float t)
        {
            return a * (1.0f - t) + (b * t);
        }
        
        void OnUpdate(float ts)
        {
            if (m_Player == null)
                return;
            Vector2 playerVelocity = m_Player.GetComponent<RigidBody2DComponent>().LinearVelocity;
            float target = DistanceFromPlayer + playerVelocity.Length();
            
            distanceFromPlayer = Lerp(distanceFromPlayer, target, 0.15f);
            
            Translation = new Vector3(m_Player.Translation.XY, distanceFromPlayer); 
        }
    }
}