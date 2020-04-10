using System.Runtime.InteropServices;

namespace hyperdbg_gui.KernelAffairs
{
    class CtrlNativeCallbacks
    {
        [DllImport("HPRDBGCTRL.dll")]
        private static extern void HyperdbgSetTextMessageCallback(Callback fn);

        public delegate int Callback(string text);
        private static Callback mInstance;   // Ensure it doesn't get garbage collected

        public static void SetCallback(Callback callback)
        {
            HyperdbgSetTextMessageCallback(callback);
        }

    }
}
