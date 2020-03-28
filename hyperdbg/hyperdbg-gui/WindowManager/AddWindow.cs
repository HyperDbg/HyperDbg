using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace hyperdbg_gui.WindowManager
{
    class AddWindow
    {
        // return is an id which describes the control
        public static int CreateCommandWindow(Form MdiParrent)
        {
            // Create an id for the window 
            int WindowId = hyperdbg_gui.Details.GlobalVariables.CurrentWindowId;
            hyperdbg_gui.Details.GlobalVariables.CurrentWindowId++;

            // Create and instance of command window
            hyperdbg_gui.Details.GlobalVariables.WindowsAttributes commandWindow = new Details.GlobalVariables.WindowsAttributes();
            commandWindow.WindowForm = new CommandWindow();
            commandWindow.WindowId = WindowId;
            commandWindow.WindowIsHidden = false;
            commandWindow.WindowTitle = "Main Command Window";
            commandWindow.WindowType = Details.GlobalVariables.WindowTypes.Command;

            // Add it to the global list of Windows
            hyperdbg_gui.Details.GlobalVariables.ListOfWindows.Add(commandWindow);

            // Show command View
            commandWindow.WindowForm.MdiParent = MdiParrent;

            commandWindow.WindowForm.Show();

            return WindowId;
        }
    }
}
