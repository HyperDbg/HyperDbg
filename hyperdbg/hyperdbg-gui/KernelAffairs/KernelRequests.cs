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
