using System;
using System.Windows.Forms;

namespace hyperdbg_gui
{
    public partial class AttachWindow : Form
    {
        public AttachWindow()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            RecentSessions r = new RecentSessions();
            r.ShowDialog();
        }
    }
}
