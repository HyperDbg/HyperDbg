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
        internal static extern int HyperdbgInit();

        public static void HyperdbgInitializer()
        {
            HyperdbgInit();
        }



    }
}
