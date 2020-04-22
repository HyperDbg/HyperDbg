namespace hyperdbg_gui
{
    partial class CommandSection
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.richTextBox2 = new System.Windows.Forms.RichTextBox();
            this.commandText = new System.Windows.Forms.RichTextBox();
            this.SuspendLayout();
            // 
            // richTextBox2
            // 
            this.richTextBox2.Dock = System.Windows.Forms.DockStyle.Left;
            this.richTextBox2.Location = new System.Drawing.Point(0, 0);
            this.richTextBox2.Margin = new System.Windows.Forms.Padding(4);
            this.richTextBox2.Name = "richTextBox2";
            this.richTextBox2.ReadOnly = true;
            this.richTextBox2.Size = new System.Drawing.Size(212, 38);
            this.richTextBox2.TabIndex = 1;
            this.richTextBox2.Text = " 0: HyperDbg >";
            // 
            // commandText
            // 
            this.commandText.Dock = System.Windows.Forms.DockStyle.Fill;
            this.commandText.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.commandText.Location = new System.Drawing.Point(212, 0);
            this.commandText.Margin = new System.Windows.Forms.Padding(4);
            this.commandText.Name = "commandText";
            this.commandText.Size = new System.Drawing.Size(1216, 38);
            this.commandText.TabIndex = 0;
            this.commandText.Text = "";
            // 
            // CommandSection
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(12F, 25F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.commandText);
            this.Controls.Add(this.richTextBox2);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "CommandSection";
            this.Size = new System.Drawing.Size(1428, 38);
            this.ResumeLayout(false);

        }

        #endregion
        public System.Windows.Forms.RichTextBox commandText;
        public System.Windows.Forms.RichTextBox richTextBox2;
    }
}
