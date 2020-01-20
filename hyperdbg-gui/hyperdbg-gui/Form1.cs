using MetroSet_UI.Forms;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace MetroSet_UI_Example
{
    public partial class Form1 : MetroSetForm
    {
        public Form1()
        {
            InitializeComponent();
        }


        private void MetroSetButton3_Click(object sender, EventArgs e)
        {
            MetroSetMessageBox.Show(this, "A new update available, do you want to update it now ?", "Available Update", MessageBoxButtons.YesNo);
        }

        private void MetroSetButton4_Click(object sender, EventArgs e)
        {
            MetroSetMessageBox.Show(this, "A new update available, do you want to update it now ?", "Available Update", MessageBoxButtons.YesNo, MessageBoxIcon.Stop);
        }

        private void MetroSetButton5_Click(object sender, EventArgs e)
        {
            MetroSetMessageBox.Show(this, "A new update available, do you want to update it now ?", "Available Update", MessageBoxButtons.YesNo, MessageBoxIcon.Information);
        }

        private void MetroSetButton6_Click(object sender, EventArgs e)
        {
            MetroSetMessageBox.Show(this, "A new update available, do you want to update it now ?", "Available Update", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
        }

        private void MetroSetButton7_Click_1(object sender, EventArgs e)
        {
            MetroSetMessageBox.Show(this, "A new update available, do you want to update it now ?", "Available Update", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
        }


        private void metroSetSwitch5_SwitchedChanged(object sender)
        {
                if (styleManager1.Style == MetroSet_UI.Design.Style.Light)
                {
                    styleManager1.Style = MetroSet_UI.Design.Style.Dark;
                }
                else
                {
                    styleManager1.Style = MetroSet_UI.Design.Style.Light;
                }
            
        }

        private void metroSetButton8_Click(object sender, EventArgs e)
        {
            styleManager1.OpenTheme();
        }
    }
}
