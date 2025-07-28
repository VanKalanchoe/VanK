namespace VanK
{
    // c#
    // struct -> stack allocated
    // class -> heap allocated (+GC)
    public struct Vector2
    {
        public float X, Y;

        public static Vector2 Zero => new Vector2(0.0f);

        public Vector2(float scalar)
        {
            this.X = scalar;
            this.Y = scalar;
        }
        
        public Vector2(float x, float y)
        {
            this.X = x;
            this.Y = y;
        }
        
        public static Vector2 operator+(Vector2 a, Vector2 b)
        {
            return new Vector2(a.X + b.X, a.Y + b.Y);
        }

        public static Vector2 operator*(Vector2 vector, float scalar)
        {
            return new Vector2(vector.X * scalar, vector.Y * scalar);
        }
    }
    
    
}