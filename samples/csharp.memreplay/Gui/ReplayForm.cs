using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace csharp.memreplay
{
    public partial class ReplayForm : Form
    {
        private readonly Replay _replay = null;

        public ReplayForm()
        {
            InitializeComponent();
        }

        public ReplayForm(Replay replay) : this()
        {
            _replay = replay;

            this.Text = _replay.Store;

            this.browser.SetRamBitmap(_replay.RamBitmap);

            // Indicators do nothing in this version
            this.browser.AddIndicator(new RamIndicator());
            this.browser.AddIndicator(new RamIndicator());
        }

        private void refreshTimer_Tick(object sender, EventArgs e)
        {
            this.browser.RefreshControl(true);
        }

    }
}
