using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Drawing.Imaging;

namespace csharp.memreplay
{

    public abstract class RamIndicatorBase
    {
        public Rectangle HorizontalBarBounds { get; private set; }
        public Rectangle VerticalBarBounds { get; private set; }

        public abstract int Dimension { get; }

        public void SetBounds(int width, int height)
        {
            HorizontalBarBounds = new Rectangle(0, 0, width, Dimension);
            VerticalBarBounds = new Rectangle(0, 0, Dimension, height);
        }
    }

    public class RamIndicator : RamIndicatorBase
    {
        public override int Dimension
        {
            get { return 10; }
        }
    }

    public partial class RamBrowser : Control, IDisposable
    {
        private class IndicatorEntry
        {
            public RamIndicatorBase Indicator;

            // Absolute bounds of the indicator
            public Rectangle Bounds;

            public Rectangle LeftBar;
            public Rectangle RightBar;
            public Rectangle TopBar;
            public Rectangle BottomBar;

            public IndicatorEntry(RamIndicatorBase indicator)
            {
                Indicator = indicator;

                Bounds = Rectangle.Empty;

                LeftBar = Rectangle.Empty;
                RightBar = Rectangle.Empty;
                TopBar = Rectangle.Empty;
                BottomBar = Rectangle.Empty;
            }
        }

        static string[] _endings = new string[] { "b", "Kb", "Mb", "Gb" };

        private const int _borderWidth = 92;
        private const int _borderHeight = 48;

        private const int _innerLineWidth = 2;
        private const int _tickTextDist = 2;
        private const int _minorTick = 32;
        private const int _majorTick = _minorTick * 2;
        private const int _majorTickLength = 5;
        private const int _minorTickLength = 3;

        private bool _disposed = false;

        private Bitmap _canvas;

        private int _width;
        private int _height;

        private int _indicatorAxisWidth;
        private int _axisWidth;
        private int _axisHeight;

        private Rectangle _innerBounds;
        private Rectangle _outerBounds;

        private List<IndicatorEntry> _indicators;
        private RamBitmap _ramBitmap;

        public RamBrowser()
        {
            _indicators = new List<IndicatorEntry>();

            ResizeRedraw = true;
        }

        protected override void Dispose(bool disposing)
        {
            if (_disposed) {
                return;
            }

            if (disposing) {

            }

            if (_canvas != null) {
                _canvas.Dispose();
                _canvas = null;
            }

            _disposed = true;
            base.Dispose(disposing);
        }

        protected override void OnPaintBackground(PaintEventArgs pevent)
        {
            Graphics g = pevent.Graphics;
            Rectangle clip = pevent.ClipRectangle;

            if ((_canvas == null) ||
                (_innerBounds.Width < 1) || (_innerBounds.Height < 1)) {
                // Draw a cross box, when the control is too small
                g.Clear(Color.White);

                using (Pen redPen = new Pen(Color.Red, 1.0f)) {
                    g.DrawLine(redPen, 0, 0, Width, Height);
                    g.DrawLine(redPen, 0, Height, Width, 0);
                }

                return;
            }

            g.DrawImage(_canvas, clip, clip, GraphicsUnit.Pixel);
        }

        protected override void OnSizeChanged(EventArgs e)
        {
            base.OnSizeChanged(e);

            _setBounds(Width, Height);
        }

        private void _setBounds(int width, int height)
        {
            // Update width of the entire indicator axis
            // We have n indicators, each one separated by a 1 pixel line
            int indicatorDim = 0;

            foreach (var entry in _indicators) {
                indicatorDim += entry.Indicator.Dimension;
            }

            _indicatorAxisWidth = indicatorDim + _indicators.Count * 1;

            // Width and height of the border and axis
            // The border contains the ticks and text, the indicator axis and
            // a 2 pixel line separating the axis from the ram view
            _axisWidth = _borderWidth + _indicatorAxisWidth + _innerLineWidth;
            _axisHeight = _borderHeight + _indicatorAxisWidth + _innerLineWidth;

            // We scale the plot only in units of major ticks to
            // always keep the axis frame consistent.
            _width = width - ((width - _axisWidth * 2) % _majorTick);
            _height = height - ((height - _axisHeight * 2) % _majorTick);

            // The inner rectangle into which we will draw the ram
            // excluding the inner frame line
            _innerBounds = new Rectangle(_axisWidth, _axisHeight,
                _width - _axisWidth * 2, _height - _axisHeight * 2);

            // The outer rectangle that we take as basis for the tick
            // and text drawing. The rectangle does not include the outer
            // frame line.
            _outerBounds = new Rectangle(_borderWidth, _borderHeight,
                _width - _borderWidth * 2, _height - _borderHeight * 2);

            // Now, that we know the dimensions of the inner view, we can
            // update the bounds of the indicators.
            for (int i = 0; i < _indicators.Count; ++i) {
                IndicatorEntry e = _indicators[i];
                int dim = e.Indicator.Dimension;

                e.Indicator.SetBounds(_innerBounds.Width, _innerBounds.Height);

                e.TopBar = new Rectangle(_innerBounds.Left,
                    _innerBounds.Top - _innerLineWidth - (dim * (i + 1)) - i,
                    _innerBounds.Width, dim);

                e.BottomBar = new Rectangle(_innerBounds.Left,
                    _innerBounds.Bottom + _innerLineWidth + (dim * i) + i,
                    _innerBounds.Width, dim);

                e.LeftBar = new Rectangle(
                    _innerBounds.Left - _innerLineWidth - (dim * (i + 1)) - i,
                    _innerBounds.Top, dim, _innerBounds.Height);

                e.RightBar = new Rectangle(
                    _innerBounds.Right + _innerLineWidth + (dim * i) + i,
                    _innerBounds.Top, dim, _innerBounds.Height);

                e.Bounds = new Rectangle(e.LeftBar.Left, e.TopBar.Top,
                    e.RightBar.Right - e.LeftBar.Left,
                    e.BottomBar.Bottom - e.TopBar.Top);
            }

            if (_ramBitmap != null) {
                _ramBitmap.SetBounds(_innerBounds.Width, _innerBounds.Height);
            }

            _setupCanvas(width, height);
        }

        private void _setupCanvas(int width, int height)
        {
            if (_canvas != null) {
                _canvas.Dispose();
                _canvas = null;
            }

            if ((_innerBounds.Width < 1) || (_innerBounds.Height < 1)) {
                return;
            }

            // We draw the control off-screen
            _canvas = new Bitmap(width, height, PixelFormat.Format32bppRgb);

            _draw(false);
        }

        private void _draw(bool drawRamOnly)
        {
            if (_canvas == null) {
                return;
            }

            using (Graphics g = Graphics.FromImage(_canvas))
            using (Pen thinPen = new Pen(Color.Black, 1.0f))
            using (Pen thickPen = new Pen(Color.Black, 2.0f))
            using (Font tickFont = new Font("Arial", 14, FontStyle.Regular,
                GraphicsUnit.Pixel))
            using (Brush fontBrush = new SolidBrush(Color.Black)) {
                Rectangle drawRect;

                if (!drawRamOnly) {
                    g.Clear(Color.White);

                    // Draw the inner border line
                    drawRect = new Rectangle(_innerBounds.Left - 1,
                        _innerBounds.Top - 1, _innerBounds.Width + 2,
                        _innerBounds.Height + 2);

                    g.DrawRectangle(thickPen, drawRect);

                    // Draw a border for each indicator
                    foreach (var entry in _indicators) {
                        drawRect = new Rectangle(entry.Bounds.Left - 1,
                            entry.Bounds.Top - 1, entry.Bounds.Width + 1,
                            entry.Bounds.Height + 1);

                        g.DrawRectangle(thinPen, drawRect);
                    }

                    // Draw x ticks
                    for (int x = 0; x <= _innerBounds.Width; x += _minorTick) {
                        int tickLength = ((x % _majorTick) == 0) ?
                            _majorTickLength : _minorTickLength;

                        // Draw top ticks
                        g.DrawLine(thinPen, _innerBounds.Left + x, _outerBounds.Top,
                            _innerBounds.Left + x, _outerBounds.Top - tickLength);

                        // Draw bottom ticks
                        g.DrawLine(thinPen, _innerBounds.Left + x, _outerBounds.Bottom,
                            _innerBounds.Left + x, _outerBounds.Bottom + tickLength);
                    }

                    // Draw y ticks
                    for (int y = 0; y <= _innerBounds.Height; y += _minorTick) {
                        int tickLength = ((y % _majorTick) == 0) ?
                            _majorTickLength : _minorTickLength;

                        // Draw left ticks
                        g.DrawLine(thinPen, _outerBounds.Left, _innerBounds.Top + y,
                            _outerBounds.Left - tickLength, _innerBounds.Top + y);

                        // Draw right ticks
                        g.DrawLine(thinPen, _outerBounds.Right, _innerBounds.Top + y,
                            _outerBounds.Right + tickLength, _innerBounds.Top + y);
                    }
                }

                if (_ramBitmap != null) {
                    _ramBitmap.Refresh();

                    g.DrawImage(_ramBitmap.Surface, _innerBounds.Left, _innerBounds.Top);
                }

                if (drawRamOnly) {
                    Invalidate(_innerBounds);
                } else {
                    Invalidate();
                }
            }
        }

        static private string _offsetToString(ulong offset)
        {
            int e = 0;
            while ((e < _endings.Length) && (offset >= 1024)) {
                e++;
                offset = offset >> 10;
            }

            return String.Format("{0}{1}", offset, _endings[e]);
        }

        public void AddIndicator(RamIndicatorBase indicator)
        {
            if (indicator == null) {
                throw new ArgumentNullException("indicator");
            }

            _indicators.Add(new IndicatorEntry(indicator));

            // By adding a new indicator the layout of the plot needs an update
            _setBounds(Width, Height);
        }

        public void SetRamBitmap(RamBitmap ram)
        {
            _ramBitmap = ram;

            if (ram == null) {
                return;
            }

            ram.SetBounds(_innerBounds.Width, _innerBounds.Height);

            _draw(true);
        }

        public void RefreshControl(bool ramOnly)
        {
            _draw(ramOnly);
        }
    }
}
