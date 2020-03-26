using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace hyperdbg_gui
{

    public partial class MainPanel : Form
    {
        /// <summary>
        /// Global Variables
        /// </summary>
        /// 
        bool StateMessageWindow = false;
        //Create a new instance of the MDI child template for Command Window
        CommandWindow commandWindow = new CommandWindow();

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
 
                StateMessageWindow = true;

                //Set parent form for the child window 
                commandWindow.MdiParent = this;

                commandWindow.Dock = DockStyle.Bottom;

                //Display the child window
                commandWindow.Show();

        }

        private void toolStripButton11_Click(object sender, EventArgs e)
        {
            foreach (Control c in this.Controls)
            {
                UpdateColorControls(c);
            }
        }
        public void UpdateColorControls(Control myControl)
        {
            myControl.BackColor = Color.FromArgb(37,37,38);
            myControl.ForeColor = Color.White;
            foreach (Control subC in myControl.Controls)
            {
                UpdateColorControls(subC);
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
    }
}
