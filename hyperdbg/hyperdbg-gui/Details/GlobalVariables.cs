using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
        public struct WindowsAttributes
        {
            public int WindowId;
            public string WindowTitle;
            public WindowTypes WindowType;
            public bool WindowIsHidden;
            public Form WindowForm;
        }

        public static List<WindowsAttributes> ListOfWindows = new List<WindowsAttributes>();
        public static int CurrentWindowId = 0;
        public static bool IsInDarkMode = false;
        public static bool IsDriverLoaded = false;
    }
}
