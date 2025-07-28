namespace VanK
{
    public class Input
    {
        public static bool IsKeyDown(ScanCode scanCode)
        {
            return InternalCalls.Input_IsKeyDown(scanCode);
        }
    }
}