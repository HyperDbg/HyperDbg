using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace hyperdbg_gui.KernelmodeRequests
{
    class KernelRequests
    {
        [DllImport("HPRDBGCTRL.dll")]
        internal static extern int HyperdbgLoad();


        [DllImport("HPRDBGCTRL.dll")]
        internal static extern int HyperdbgUnload();

        public static void HyperdbgLoader()
        {
            HyperdbgLoad();
        }

        public static void HyperdbgUnloader()
        {
            HyperdbgUnload();
        }
    }
}
