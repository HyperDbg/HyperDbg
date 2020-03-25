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

    public partial class Form1 : Form
    {
        /// <summary>
        /// Global Variables
        /// </summary>
        /// 
        bool StateMessageWindow = false;
        //Create a new instance of the MDI child template for Command Window
        CommandWindow commandWindow = new CommandWindow();

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            this.Text = "hyperdbg debugger (" + hyperdbg_gui.Details.HyperdbgInformation.HyperdbgVersion + ") x86-64";
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
    }
}
