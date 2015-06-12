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
    public partial class MainForm : Form
    {
        private readonly Replay _replay = null;
        private long numProcessedLast = 0;

        public MainForm()
        {
            InitializeComponent();
        }

        public MainForm(Replay replay) : this()
        {
            _replay = replay;

            serverLabel.Text = _replay.Server;
            storeLabel.Text = _replay.Store;
            writeStreamLabel.Text = _replay.WriteStream;

            totalWritesLabel.Text = String.Format("{0:#,0}",
                _replay.WriteStreamNumEntries);
            writeSizeLabel.Text = String.Format("{0:#,0} bytes",
                _replay.WriteStreamNumEntries * 32);
            writeComprSizeLabel.Text = String.Format("{0:#,0} bytes",
                _replay.WriteStreamSizeCompressed);

            progressBar1.Maximum = 100000;

            // Start the replay
            playPauseButton.ImageIndex = 1;
            _replay.Start();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            // Simulation
            cycleLabel.Text = String.Format("{0:#,0}", _replay.Cycle);

            // Stats
            writesLabel.Text = String.Format("{0:#,0}", _replay.NumWrites.Sum());
            writes1Label.Text = String.Format("{0:#,0}", _replay.NumWrites[0]);
            writes2Label.Text = String.Format("{0:#,0}", _replay.NumWrites[1]);
            writes4Label.Text = String.Format("{0:#,0}", _replay.NumWrites[2]);
            writes8Label.Text = String.Format("{0:#,0}", _replay.NumWrites[3]);

            writtenDataLabel.Text = String.Format("{0:#,0} bytes",
                                        _replay.NumWrites[0] +
                                        _replay.NumWrites[1] * 2 +
                                        _replay.NumWrites[2] * 4 +
                                        _replay.NumWrites[3] * 8);

            cr3SwitchesLabel.Text = String.Format("{0:#,0}", _replay.NumCr3Switches);

            // Replay
            TimeSpan replayTime = _replay.ReplayTime;
            replayTimeLabel.Text = replayTime.ToString();

            long numProcessed = _replay.NumWrites.Sum();
            long numTotal     = (long)_replay.WriteStreamNumEntries;

            processedDataLabel.Text = String.Format("{0:#,0} bytes",
                                        numProcessed * 32);

            long speed = ((numProcessed - numProcessedLast) * 32) * (1000 / timer1.Interval);
            double secs = (replayTime.TotalSeconds > 0) ? replayTime.TotalSeconds : 1.0;
            numProcessedLast = numProcessed;

            curSpeedLabel.Text = String.Format("{0:#,0} bytes/s", speed);
            avgSpeedLabel.Text = String.Format("{0:#,0} bytes/s",
                                    (long)((numProcessed * 32) / secs));

            progressBar1.Value = (int)((numProcessed * 100000) / numTotal);

            if (_replay.IsDone) {
                playPauseButton.Enabled = false;
                singleStepButton.Enabled = false;
                timer1.Enabled = false;
            }
        }

        private void playPauseButton_Click(object sender, EventArgs e)
        {
            if (playPauseButton.ImageIndex == 0) {
                singleStepButton.Enabled = false;
                playPauseButton.ImageIndex = 1;

                _replay.Start();
            } else {
                singleStepButton.Enabled = true;
                playPauseButton.ImageIndex = 0;

                _replay.Suspend();
            }
        }

        private void singleStepButton_Click(object sender, EventArgs e)
        {
            _replay.SingleStep();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }
    }
}
