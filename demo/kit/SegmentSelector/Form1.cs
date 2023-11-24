using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace OSDevToolDptr
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            comboRing.SelectedIndex = comboBoxT.SelectedIndex = 0;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (textBoxBaseAddr.Text.Length != 8)
            {
                textBoxDescrib.Text = "段基地址格式错误";
                textBoxBaseAddr.Focus();
                return;
            }
            if (textBoxLimit.Text.Length != 5)
            {
                textBoxDescrib.Text = "段界限格式错误";
                textBoxLimit.Focus();
                return;
            }
            /*段存在位=1 64位标识=0 自定义位=0*/
            textBoxA.Text = textBoxBaseAddr.Text.ToUpper().Substring(0, 2) +
                (!checkBoxG.Checked && !checkBox32.Checked ? "0" :
                (!checkBoxG.Checked && checkBox32.Checked ? "4" :
                (checkBoxG.Checked && !checkBox32.Checked ? "8" : "C"))) +
                textBoxLimit.Text.ToUpper().Substring(0, 1) +
                (comboRing.SelectedIndex == 0 && !checkNotSys.Checked ? "8" :
                (comboRing.SelectedIndex == 0 && checkNotSys.Checked ? "9" :
                (comboRing.SelectedIndex == 1 && !checkNotSys.Checked ? "A" :
                (comboRing.SelectedIndex == 1 && checkNotSys.Checked ? "B" :
                (comboRing.SelectedIndex == 2 && !checkNotSys.Checked ? "C" :
                (comboRing.SelectedIndex == 2 && checkNotSys.Checked ? "D" :
                (comboRing.SelectedIndex == 3 && !checkNotSys.Checked ? "E" : "F"))))))) +
                (!checkCode.Checked ? "0246"[comboBoxT.SelectedIndex] : "8ACE"[comboBoxT.SelectedIndex]) +
                textBoxBaseAddr.Text.ToUpper().Substring(2, 2);

            textBoxB.Text = textBoxBaseAddr.Text.ToUpper().Substring(4) + textBoxLimit.Text.ToUpper().Substring(1);
            PutState();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (textBoxA.Text.Length != 8) 
            {
                textBoxDescrib.Text = "描述符A错误";
                return;
            }
            if (textBoxB.Text.Length != 8) 
            {
                textBoxDescrib.Text = "描述符B错误";
                return;
            }
            textBoxBaseAddr.Text = textBoxA.Text.Substring(0, 2) + textBoxA.Text.Substring(6) + textBoxB.Text.Substring(0, 4);
            textBoxLimit.Text = textBoxA.Text.Substring(3, 1) + textBoxB.Text.Substring(4, 4);
            char c =textBoxA.Text.ToUpper()[2];
            checkBoxG.Checked = ((c == '8' || c == '9' || c == 'A' || c == 'B' ||
                c == 'C' || c == 'D' || c == 'E' || c == 'F') ? true : false);

            checkBox32.Checked = ((c == '4' || c == '5' || c == '6' || c == '7' ||
                c == 'C' || c == 'D' || c == 'E' || c == 'F') ? true : false);
            c =textBoxA.Text.ToUpper()[4];
            checkNotSys.Checked = ((c == '1' || c == '3' || c == '5' || c == '7' ||
                c == '9' || c == 'B' || c == 'D' || c == 'F') ? true : false);
            comboRing.SelectedIndex = ((c == '0' || c == '1' || c == '8' || c == '9') ? 0 :
                ((c == '2' || c == '3' || c == 'A' || c == 'B') ? 1 :
                ((c == '4' || c == '5' || c == 'C' || c == 'D') ? 2 : 3)));

            c = textBoxA.Text.ToUpper()[5];
            checkCode.Checked = ((c == '8' || c == '9' || c == 'A' || c == 'B' ||
                c == 'C' || c == 'D' || c == 'E' || c == 'F') ? true : false);
            comboBoxT.SelectedIndex = ((c == '0' || c == '1' || c == '8' || c == '9') ? 0 :
                ((c == '2' || c == '3' || c == 'A' || c == 'B') ? 1 :
                ((c == '4' || c == '5' || c == 'C' || c == 'D') ? 2 : 3)));
            PutState();
        }
        private void PutState()
        {
            textBoxDescrib.Text = "BASE[" + textBoxBaseAddr.Text + "]LIM[" + textBoxLimit.Text +
                "]" + (checkBoxG.Checked ? "[4KB]" : "[Byt]") + (checkBox32.Checked ? "[32]" : "[16]") +
                (checkNotSys.Checked ? "[KiSys]" : "[NaSys]") + (checkCode.Checked ? "[CODE]" : "[DATA]") +
                "[" + comboRing.Text + "][" + comboBoxT.Text + "]";
        }
    }
}
