using System;

using VanK;

namespace Sandbox
{
    public class Camera : Entity
    {
        public Entity otherEntity;
        
        public float DistanceFromPlayer = 5.0f;

        private Entity m_Player;
        
        void OnCreate()
        {
            m_Player = FindEntityByName("Player");
        }
        
        void OnUpdate(float ts)
        {
            if (m_Player != null)
                Translation = new Vector3(m_Player.Translation.XY, DistanceFromPlayer); 
            //Player player = FindEntityByName("Player").As<Player>();
            
            float speed = 1.0f;
            Vector3 velocity = Vector3.Zero;
            
            if (Input.IsKeyDown(ScanCode.SDL_SCANCODE_UP))
                velocity.Y = 1.0f;
            else if (Input.IsKeyDown(ScanCode.SDL_SCANCODE_DOWN))
                velocity.Y = -1.0f;
            
            if (Input.IsKeyDown(ScanCode.SDL_SCANCODE_LEFT))
                velocity.X = -1.0f;
            else if (Input.IsKeyDown(ScanCode.SDL_SCANCODE_RIGHT))
                velocity.X = 1.0f;
            
            velocity *= speed;
            DistanceFromPlayer += ts;
            
            Vector3 translation = Translation;
            translation += velocity * ts;
            Translation = translation;
        }
    }
}