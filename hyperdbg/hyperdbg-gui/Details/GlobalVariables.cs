using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

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
