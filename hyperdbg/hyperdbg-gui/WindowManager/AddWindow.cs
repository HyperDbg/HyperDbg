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
        public static void CreateCommandWindow(Form MdiParrent)
        {
            // Make it globally available
            hyperdbg_gui.Details.GlobalVariables.CommandWindow = new CommandWindow();

            // Show command View
            hyperdbg_gui.Details.GlobalVariables.CommandWindow.MdiParent = MdiParrent;

            hyperdbg_gui.Details.GlobalVariables.CommandWindow.Show();

        }
    }
}
