using System;
using System.Windows.Forms;

namespace hyperdbg_gui
{
    public partial class child : Form
    {
        public child()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            MessageBox.Show("Test");
        }
    }
}
