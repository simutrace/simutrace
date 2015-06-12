using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace csharp.memreplay
{
    public class RamBitmap : IDisposable
    {
        private bool _disposed = false;

        private int _width;
        private int _height;

        private RamMap _ram;
        private Bitmap _bitmap;

        // Start address from which to display data in the bitmap
        private ulong _start;

        // Zoom level in number of frames that compose a single pixel
        private uint _zoomLevel;

        public RamBitmap(RamMap ram)
        {
            if (ram == null) {
                throw new ArgumentNullException("ram");
            }

            _ram = ram;

            _width = 0;
            _height = 0;

            _start = 0;
            _zoomLevel = 1;
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (_disposed) {
                return;
            }

            if (disposing) {

            }

            SetBounds(0, 0);

            _disposed = true;
        }

        public static Color _blend(Color color, Color backColor, double amount)
        {
            byte r = (byte)((color.R * amount) + backColor.R * (1 - amount));
            byte g = (byte)((color.G * amount) + backColor.G * (1 - amount));
            byte b = (byte)((color.B * amount) + backColor.B * (1 - amount));
            return Color.FromArgb(r, g, b);
        }

        private void _draw()
        {
            const int fadeMax = 0x4FFFFF;

            BitmapData bd = _bitmap.LockBits(new Rectangle(0, 0, _width, _height),
                ImageLockMode.ReadWrite, PixelFormat.Format32bppRgb);

            try {
                Color coldFrame = Color.FromArgb(0xFF, 0xBB, 0xBB, 0xBB);
                Color hotWriteFrame = Color.FromArgb(0xFF, 0xE5, 0x14, 0x00);

                unsafe {
                    int* px = (int*)bd.Scan0;
                    int* end = px + _width * _height;

                    ulong index = _ram.Index;
                    ulong addr = _start;
                    ulong size = _ram.Size;
                    FrameMetadata[] frames = _ram.Frames;
                    while ((addr < size) && (px < end)) {

                        // Depending on the zoom level, we need to interpolate
                        // a certain number of sub frames
                        uint startframe = (uint)(addr >> 12);

                        long wdelta = 0;
                        int wcount = 0;
                        for (uint i = 0; i < _zoomLevel; ++i) {
                            ulong lw = frames[startframe + i].LastWriteAccess;
                            if (lw > 0) {
                                wdelta += (long)(index - lw);

                                wcount++;
                            }
                        }

                        if (wcount == 0) {
                            *px = Color.White.ToArgb();
                        } else {
                            wdelta = fadeMax - wdelta / wcount;
                            if (wdelta < 0) {
                                *px = coldFrame.ToArgb();
                            } else {
                                *px = _blend(hotWriteFrame, coldFrame,
                                    (double)wdelta / (double)fadeMax).ToArgb();
                            }
                        }

                        px++;
                        addr += 4096 * _zoomLevel;
                    }
                }

            } finally {
                _bitmap.UnlockBits(bd);
            }
        }

        public void Refresh()
        {
            _draw();
        }

        public void SetBounds(int width, int height)
        {
            _width = width;
            _height = height;

            if (_bitmap != null) {
                _bitmap.Dispose();
                _bitmap = null;
            }

            if (_width <= 0 || _height <= 0) {
                return;
            }

            _bitmap = new Bitmap(width, height, PixelFormat.Format32bppRgb);

            using (Graphics g = Graphics.FromImage(_bitmap)) {
                g.Clear(Color.FromArgb(0xFF, 0x22, 0x22, 0x22));
            }

            _draw();
        }

        public void Save(string filename)
        {
            _bitmap.Save(filename);
        }

        public int Width
        {
            get { return _width; }
        }

        public int Height
        {
            get { return _height; }
        }

        public Bitmap Surface
        {
            get { return _bitmap; }
        }
    }
}
