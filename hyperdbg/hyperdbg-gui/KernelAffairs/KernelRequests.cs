using System.Runtime.InteropServices;

namespace hyperdbg_gui.KernelmodeRequests
{
    class KernelRequests
    {
        [DllImport("HPRDBGCTRL.dll")]
        internal static extern int HyperdbgLoad();


        [DllImport("HPRDBGCTRL.dll")]
        internal static extern int HyperdbgUnload();

        [DllImport("HPRDBGCTRL.dll")]
        internal static extern int HyperdbgInstallDriver();

        [DllImport("HPRDBGCTRL.dll")]
        internal static extern int HyperdbgUninstallDriver();

        [DllImport("HPRDBGCTRL.dll")]
        internal static extern int HyperdbgInterpreter(string command);


        public static int HyperdbgCommandInterpreter(string command)
        {
            return HyperdbgInterpreter(command);
        }

        public static int HyperdbgLoader()
        {
            return HyperdbgLoad();
        }
        public static int HyperdbgDriverInstaller()
        {
            return HyperdbgInstallDriver();
        }
        public static int HyperdbgDriverUninstaller()
        {
            return HyperdbgUninstallDriver();
        }

        public static void HyperdbgUnloader()
        {
            HyperdbgUnload();
        }
    }
}
