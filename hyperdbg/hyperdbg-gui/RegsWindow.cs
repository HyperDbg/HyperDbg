using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace hyperdbg_gui
{

    public partial class RegsWindow : Form
    {
        public RegsWindow()
        {
            InitializeComponent();
        }

        private void CopyControl(Control sourceControl, Control targetControl)
        {
            // make sure these are the same
            if (sourceControl.GetType() != targetControl.GetType())
            {
                throw new Exception("Incorrect control types");
            }

            foreach (PropertyInfo sourceProperty in sourceControl.GetType().GetProperties())
            {
                object newValue = sourceProperty.GetValue(sourceControl, null);

                MethodInfo mi = sourceProperty.GetSetMethod(true);
                if (mi != null)
                {
                    sourceProperty.SetValue(targetControl, newValue, null);
                }
            }
        }
        string Registers = "rax rcx rdx rbx rsp rbp rsi rdi r8 r9 r10 r11 r12 r13 r14 r15" +
            " rip efl cs ds es fs gs ss dr0 dr1 dr2 dr3 dr6 dr7 fpcw fpsw fptw st0 st1 st2" +
            " st3 st4 st5 st6 st7 mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7 mxcsr xmm0 xmm1 xmm2 xmm3" +
            " xmm4 xmm5 xmm6 xmm7 xmm8 xmm9 xmm10 xmm11 xmm12 xmm13 xmm14 xmm15 xmm0/0 xmm0/1" +
            " xmm0/2 xmm0/3 xmm1/0 xmm1/1 xmm1/2 xmm1/3 xmm2/0 xmm2/1 xmm2/2 xmm2/3 xmm3/0 xmm3/1" +
            " xmm3/2 xmm3/3 xmm4/0 xmm4/1 xmm4/2 xmm4/3 xmm5/0 xmm5/1 xmm5/2 xmm5/3 xmm6/0 xmm6/1" +
            " xmm6/2 xmm6/3 xmm7/0 xmm7/1 xmm7/2 xmm7/3 xmm8/0 xmm8/1 xmm8/2 xmm8/3 xmm9/0 xmm9/1" +
            " xmm9/2 xmm9/3 xmm10/0 xmm10/1 xmm10/2 xmm10/3 xmm11/0 xmm11/1 xmm11/2 xmm11/3 xmm12/0" +
            " xmm12/1 xmm12/2 xmm12/3 xmm13/0 xmm13/1 xmm13/2 xmm13/3 xmm14/0 xmm14/1 xmm14/2 xmm14/3" +
            " xmm15/0 xmm15/1 xmm15/2 xmm15/3 xmm0l xmm1l xmm2l xmm3l xmm4l xmm5l xmm6l xmm7l xmm8l" +
            " xmm9l xmm10l xmm11l xmm12l xmm13l xmm14l xmm15l xmm0h xmm1h xmm2h xmm3h xmm4h xmm5h" +
            " xmm6h xmm7h xmm8h xmm9h xmm10h xmm11h xmm12h xmm13h xmm14h xmm15h ymm0 ymm1 ymm2 ymm3" +
            " ymm4 ymm5 ymm6 ymm7 ymm8 ymm9 ymm10 ymm11 ymm12 ymm13 ymm14 ymm15 ymm0/0 ymm0/1 ymm0/2" +
            " ymm0/3 ymm1/0 ymm1/1 ymm1/2 ymm1/3 ymm2/0 ymm2/1 ymm2/2 ymm2/3 ymm3/0 ymm3/1 ymm3/2" +
            " ymm3/3 ymm4/0 ymm4/1 ymm4/2 ymm4/3 ymm5/0 ymm5/1 ymm5/2 ymm5/3 ymm6/0 ymm6/1 ymm6/2" +
            " ymm6/3 ymm7/0 ymm7/1 ymm7/2 ymm7/3 ymm8/0 ymm8/1 ymm8/2 ymm8/3 ymm9/0 ymm9/1 ymm9/2" +
            " ymm9/3 ymm10/0 ymm10/1 ymm10/2 ymm10/3 ymm11/0 ymm11/1 ymm11/2 ymm11/3 ymm12/0 ymm12/1" +
            " ymm12/2 ymm12/3 ymm13/0 ymm13/1 ymm13/2 ymm13/3 ymm14/0 ymm14/1 ymm14/2 ymm14/3 ymm15/0" +
            " ymm15/1 ymm15/2 ymm15/3 ymm0l ymm1l ymm2l ymm3l ymm4l ymm5l ymm6l ymm7l ymm8l ymm9l" +
            " ymm10l ymm11l ymm12l ymm13l ymm14l ymm15l ymm0h ymm1h ymm2h ymm3h ymm4h ymm5h ymm6h" +
            " ymm7h ymm8h ymm9h ymm10h ymm11h ymm12h ymm13h ymm14h ymm15h zmm0 zmm1 zmm2 zmm3 zmm4" +
            " zmm5 zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19" +
            " zmm20 zmm21 zmm22 zmm23 zmm24 zmm25 zmm26 zmm27 zmm28 zmm29 zmm30 zmm31 k0 k1 k2 k3" +
            " k4 k5 k6 k7 zmm0h zmm1h zmm2h zmm3h zmm4h zmm5h zmm6h zmm7h zmm8h zmm9h zmm10h zmm11h" +
            " zmm12h zmm13h zmm14h zmm15h exfrom exto brfrom brto eax ecx edx ebx esp ebp esi edi" +
            " r8d r9d r10d r11d r12d r13d r14d r15d eip ax cx dx bx sp bp si di r8w r9w r10w r11w" +
            " r12w r13w r14w r15w ip fl al cl dl bl spl bpl sil dil r8b r9b r10b r11b r12b r13b" +
            " r14b r15b ah ch dh bh iopl of df if tf sf zf af pf cf vip vif cr0 cr2 cr3 cr4 cr8" +
            " gdtr gdtl idtr idtl tr ldtr kmxcsr kdr0 kdr1 kdr2 kdr3 kdr6 kdr7 xcr0";



        private void RegsWindow_Load(object sender, EventArgs e)
        {
        
            List<string> RegsList = Registers.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries).ToList();

            foreach (var item in RegsList)
            {
                int rowId = dataGridView1.Rows.Add();
                DataGridViewRow row = dataGridView1.Rows[rowId];
                row.Cells["Register"].Value = item.ToString();
                row.Cells["Value"].Value = "Value2";

            }
            dataGridView1.Rows[0].Cells[0].Selected = false;

        }

    }
}
