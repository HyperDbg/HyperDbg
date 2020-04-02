using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace hyperdbg_gui
{

    public partial class MainPanel : Form
    {

        public MainPanel()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            this.Text = "hyperdbg debugger (" + hyperdbg_gui.Details.HyperdbgInformation.HyperdbgVersion + ") x86-64";
            ControlMoverOrResizer.Init(panel1);
            ControlMoverOrResizer.WorkType = ControlMoverOrResizer.MoveOrResize.Resize;


        }

        private void addNewMenuToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //Create a new instance of the MDI child template form
            child chForm = new child();

            //Set parent form for the child window 
            chForm.MdiParent = this;

            //Display the child window
            chForm.Show();
        }

        private void richTextBox1_Enter(object sender, EventArgs e)
        {

        }

        private void richTextBox1_Leave(object sender, EventArgs e)
        {

        }

        private void commandWindowsToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }

        private void toolStripButton11_Click(object sender, EventArgs e)
        {
            bool IsDark = !hyperdbg_gui.Details.GlobalVariables.IsInDarkMode;
            hyperdbg_gui.Details.GlobalVariables.IsInDarkMode = !hyperdbg_gui.Details.GlobalVariables.IsInDarkMode;
            
            foreach (Control c in this.Controls)
            {
                UpdateColorControls(c, IsDark);
            }
        }
        public void UpdateColorControls(Control myControl, bool IsDark)
        {
            if (IsDark)
            {
                myControl.BackColor = Color.FromArgb(37, 37, 38);
                myControl.ForeColor = Color.White;
            }
            else
            {
                myControl.BackColor = Color.White;
                myControl.ForeColor = Color.Black;
            }
            foreach (Control subC in myControl.Controls)
            {
                UpdateColorControls(subC, IsDark);
            }
        }

        private void registersToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //Create a new instance of the MDI child template form
            RegsWindow chForm = new RegsWindow();

            //Set parent form for the child window 
            chForm.MdiParent = this;

            //Display the child window
            chForm.Show();
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AboutWindow about = new AboutWindow();
            about.ShowDialog();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void toolStripMenuItem2_Click(object sender, EventArgs e)
        {
            MessageBox.Show("Not yet supported, support will be available in the future versions");
        }
        private int ReceivedMessagesHandler(string Text)
        {

            hyperdbg_gui.Details.GlobalVariables.CommandWindow.richTextBox1.AppendText(Text + "\n");
            return 0;
        }

        public void LoadDriver()
        {
            hyperdbg_gui.KernelAffairs.CtrlNativeCallbacks.SetCallback(ReceivedMessagesHandler);

            if (hyperdbg_gui.KernelmodeRequests.KernelRequests.HyperdbgDriverInstaller() != 0)
            {
                MessageBox.Show("There was an error installing the driver", "Error"
                    , MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;

            }
            if (hyperdbg_gui.KernelmodeRequests.KernelRequests.HyperdbgLoader() != 0)
            {
                MessageBox.Show("Failed to load hyperdbg's hypervisor driver, see logs for more information"
                    ,"Error", MessageBoxButtons.OK,MessageBoxIcon.Error);
                return;
            }
            
            hyperdbg_gui.Details.GlobalVariables.IsDriverLoaded = true;
            toolStripButton21.Image = hyperdbg_gui.Properties.Resources.Pan_Green_Circle;

        }
        public void UnloadDriver()
        {
            hyperdbg_gui.KernelmodeRequests.KernelRequests.HyperdbgUnloader();

            if (hyperdbg_gui.KernelmodeRequests.KernelRequests.HyperdbgDriverUninstaller() != 0)
            {
                MessageBox.Show("There was an error unloading the driver", "Error"
                    , MessageBoxButtons.OK, MessageBoxIcon.Error);

            }
            hyperdbg_gui.Details.GlobalVariables.IsDriverLoaded = false;
        }


        private void toolStripButton21_Click(object sender, EventArgs e)
        {
            if (!hyperdbg_gui.Details.GlobalVariables.IsDriverLoaded)
            {
                WindowManager.AddWindow.CreateCommandWindow(this);
                Details.GlobalVariables.VmxInitThread= new Thread(LoadDriver);
                Details.GlobalVariables.VmxInitThread.Start();

            }
            else
            {
                UnloadDriver();
                toolStripButton21.Image = hyperdbg_gui.Properties.Resources.Trafficlight_red_icon;
            }

        }

        private void toolStripButton6_Click(object sender, EventArgs e)
        {
            AboutWindow about = new AboutWindow();
            about.ShowDialog();
        }
    }
}
