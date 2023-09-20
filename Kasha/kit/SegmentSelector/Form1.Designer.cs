namespace OSDevToolDptr
{
    partial class Form1
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.comboBoxT = new System.Windows.Forms.ComboBox();
            this.comboRing = new System.Windows.Forms.ComboBox();
            this.button2 = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.checkCode = new System.Windows.Forms.CheckBox();
            this.checkNotSys = new System.Windows.Forms.CheckBox();
            this.checkBox32 = new System.Windows.Forms.CheckBox();
            this.checkBoxG = new System.Windows.Forms.CheckBox();
            this.textBoxLimit = new System.Windows.Forms.TextBox();
            this.textBoxBaseAddr = new System.Windows.Forms.TextBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.textBoxA = new System.Windows.Forms.TextBox();
            this.textBoxB = new System.Windows.Forms.TextBox();
            this.textBoxDescrib = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControl1.Location = new System.Drawing.Point(0, 0);
            this.tabControl1.Margin = new System.Windows.Forms.Padding(4, 5, 4, 5);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(702, 334);
            this.tabControl1.TabIndex = 0;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.comboBoxT);
            this.tabPage1.Controls.Add(this.comboRing);
            this.tabPage1.Controls.Add(this.button2);
            this.tabPage1.Controls.Add(this.button1);
            this.tabPage1.Controls.Add(this.checkCode);
            this.tabPage1.Controls.Add(this.checkNotSys);
            this.tabPage1.Controls.Add(this.checkBox32);
            this.tabPage1.Controls.Add(this.checkBoxG);
            this.tabPage1.Controls.Add(this.textBoxLimit);
            this.tabPage1.Controls.Add(this.textBoxBaseAddr);
            this.tabPage1.Controls.Add(this.panel1);
            this.tabPage1.Controls.Add(this.textBoxDescrib);
            this.tabPage1.Controls.Add(this.label2);
            this.tabPage1.Controls.Add(this.label1);
            this.tabPage1.Location = new System.Drawing.Point(4, 36);
            this.tabPage1.Margin = new System.Windows.Forms.Padding(4, 5, 4, 5);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(4, 5, 4, 5);
            this.tabPage1.Size = new System.Drawing.Size(694, 294);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "GDTDescriptor";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // comboBoxT
            // 
            this.comboBoxT.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.comboBoxT.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxT.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.comboBoxT.FormattingEnabled = true;
            this.comboBoxT.Items.AddRange(new object[] {
            "只读/只执",
            "读写/读执",
            "只读下拓/只读依从",
            "读写下拓/读执依从"});
            this.comboBoxT.Location = new System.Drawing.Point(517, 98);
            this.comboBoxT.Name = "comboBoxT";
            this.comboBoxT.Size = new System.Drawing.Size(170, 35);
            this.comboBoxT.TabIndex = 6;
            // 
            // comboRing
            // 
            this.comboRing.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboRing.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.comboRing.FormattingEnabled = true;
            this.comboRing.Items.AddRange(new object[] {
            "R0",
            "R1",
            "R2",
            "R3"});
            this.comboRing.Location = new System.Drawing.Point(401, 98);
            this.comboRing.Name = "comboRing";
            this.comboRing.Size = new System.Drawing.Size(110, 35);
            this.comboRing.TabIndex = 6;
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(153, 137);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(131, 43);
            this.button2.TabIndex = 5;
            this.button2.Text = "逆转状态";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(16, 137);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(131, 43);
            this.button1.TabIndex = 5;
            this.button1.Text = "转为描述符";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // checkCode
            // 
            this.checkCode.AutoSize = true;
            this.checkCode.Location = new System.Drawing.Point(321, 100);
            this.checkCode.Name = "checkCode";
            this.checkCode.Size = new System.Drawing.Size(74, 31);
            this.checkCode.TabIndex = 4;
            this.checkCode.Text = "代码";
            this.checkCode.UseVisualStyleBackColor = true;
            // 
            // checkNotSys
            // 
            this.checkNotSys.AutoSize = true;
            this.checkNotSys.Location = new System.Drawing.Point(205, 100);
            this.checkNotSys.Name = "checkNotSys";
            this.checkNotSys.Size = new System.Drawing.Size(114, 31);
            this.checkNotSys.TabIndex = 4;
            this.checkNotSys.Text = "非系统段";
            this.checkNotSys.UseVisualStyleBackColor = true;
            // 
            // checkBox32
            // 
            this.checkBox32.AutoSize = true;
            this.checkBox32.Location = new System.Drawing.Point(121, 100);
            this.checkBox32.Name = "checkBox32";
            this.checkBox32.Size = new System.Drawing.Size(78, 31);
            this.checkBox32.TabIndex = 4;
            this.checkBox32.Text = "32位";
            this.checkBox32.UseVisualStyleBackColor = true;
            // 
            // checkBoxG
            // 
            this.checkBoxG.AutoSize = true;
            this.checkBoxG.Location = new System.Drawing.Point(16, 100);
            this.checkBoxG.Name = "checkBoxG";
            this.checkBoxG.Size = new System.Drawing.Size(99, 31);
            this.checkBoxG.TabIndex = 4;
            this.checkBoxG.Text = "4K倍率";
            this.checkBoxG.UseVisualStyleBackColor = true;
            // 
            // textBoxLimit
            // 
            this.textBoxLimit.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxLimit.Location = new System.Drawing.Point(110, 60);
            this.textBoxLimit.Name = "textBoxLimit";
            this.textBoxLimit.Size = new System.Drawing.Size(577, 34);
            this.textBoxLimit.TabIndex = 3;
            // 
            // textBoxBaseAddr
            // 
            this.textBoxBaseAddr.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxBaseAddr.Location = new System.Drawing.Point(109, 14);
            this.textBoxBaseAddr.Name = "textBoxBaseAddr";
            this.textBoxBaseAddr.Size = new System.Drawing.Size(577, 34);
            this.textBoxBaseAddr.TabIndex = 3;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.splitContainer1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(4, 217);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(686, 38);
            this.panel1.TabIndex = 2;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.textBoxA);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.textBoxB);
            this.splitContainer1.Size = new System.Drawing.Size(686, 38);
            this.splitContainer1.SplitterDistance = 313;
            this.splitContainer1.TabIndex = 0;
            // 
            // textBoxA
            // 
            this.textBoxA.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper;
            this.textBoxA.Dock = System.Windows.Forms.DockStyle.Fill;
            this.textBoxA.Location = new System.Drawing.Point(0, 0);
            this.textBoxA.Name = "textBoxA";
            this.textBoxA.Size = new System.Drawing.Size(313, 34);
            this.textBoxA.TabIndex = 0;
            // 
            // textBoxB
            // 
            this.textBoxB.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper;
            this.textBoxB.Dock = System.Windows.Forms.DockStyle.Fill;
            this.textBoxB.Location = new System.Drawing.Point(0, 0);
            this.textBoxB.Name = "textBoxB";
            this.textBoxB.Size = new System.Drawing.Size(369, 34);
            this.textBoxB.TabIndex = 0;
            // 
            // textBoxDescrib
            // 
            this.textBoxDescrib.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.textBoxDescrib.Location = new System.Drawing.Point(4, 255);
            this.textBoxDescrib.Name = "textBoxDescrib";
            this.textBoxDescrib.ReadOnly = true;
            this.textBoxDescrib.Size = new System.Drawing.Size(686, 34);
            this.textBoxDescrib.TabIndex = 1;
            this.textBoxDescrib.Text = "Description.";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(11, 63);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(72, 27);
            this.label2.TabIndex = 0;
            this.label2.Text = "段界限";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(11, 17);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(92, 27);
            this.label1.TabIndex = 0;
            this.label1.Text = "段基地址";
            // 
            // tabPage2
            // 
            this.tabPage2.Location = new System.Drawing.Point(4, 29);
            this.tabPage2.Margin = new System.Windows.Forms.Padding(4, 5, 4, 5);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(4, 5, 4, 5);
            this.tabPage2.Size = new System.Drawing.Size(694, 301);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "tabPage2";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(12F, 27F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(702, 334);
            this.Controls.Add(this.tabControl1);
            this.Font = new System.Drawing.Font("微软雅黑", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.Margin = new System.Windows.Forms.Padding(4, 5, 4, 5);
            this.Name = "Form1";
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage1.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.TextBox textBoxDescrib;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.TextBox textBoxA;
        private System.Windows.Forms.TextBox textBoxB;
        private System.Windows.Forms.TextBox textBoxBaseAddr;
        private System.Windows.Forms.TextBox textBoxLimit;
        private System.Windows.Forms.CheckBox checkNotSys;
        private System.Windows.Forms.CheckBox checkBox32;
        private System.Windows.Forms.CheckBox checkBoxG;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.ComboBox comboRing;
        private System.Windows.Forms.ComboBox comboBoxT;
        private System.Windows.Forms.CheckBox checkCode;
    }
}

