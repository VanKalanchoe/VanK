using System;

using VanK;

namespace Sandbox
{
    public class Camera : Entity
    {
        public Entity otherEntity;
        
        void OnUpdate(float ts)
        {
            //Console.WriteLine($"Player.OnUpdate: {ts}");

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
            
            Vector3 translation = Translation;
            translation += velocity * ts;
            Translation = translation;
        }
    }
}