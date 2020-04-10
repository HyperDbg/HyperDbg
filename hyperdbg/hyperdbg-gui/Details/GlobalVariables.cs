using System.Threading;

namespace hyperdbg_gui.Details
{
    class GlobalVariables
    {
        public enum WindowTypes
        {
            Command,
            Stack,
            Memory,
            Registers,
            Callstack,

        }

        public static bool IsInDarkMode = false;
        public static bool IsDriverLoaded = false;
        public static Thread VmxInitThread;
        public static CommandWindow CommandWindow;

    }
}
