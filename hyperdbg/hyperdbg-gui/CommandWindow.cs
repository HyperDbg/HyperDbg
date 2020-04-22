using System.Windows.Forms;

namespace hyperdbg_gui
{
    public partial class CommandWindow : Form
    {
        public CommandWindow()
        {
            InitializeComponent();
        }

        private void richTextBox1_TextChanged(object sender, System.EventArgs e)
        {
            // set the current caret position to the end
            richTextBox1.SelectionStart = richTextBox1.Text.Length;
            // scroll it automatically
            richTextBox1.ScrollToCaret();

        }
    }
}
