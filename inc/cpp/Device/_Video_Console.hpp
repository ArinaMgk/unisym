// only included by _Video_Console.hpp
#ifndef _INC_DEVICE_Video_X
#error only include by _Video_Console.hpp
#endif

namespace uni {
	// inherit Console_t to make a console with self-defined font.
	class VideoConsole : public Console_t, public SheetTrait
	{
	public: const VideoControlInterface* vci = nullptr;
	protected:
		Point cursor = { 0,0 };
		Color* buffer = nullptr;// pixels/words buffer, not sheet_buffer
		Size2 size = { 0, 0 };// char unit but pixel
		stduint typ = 1;// 0: inner_8x5, 1:inner_16x8
	public:
		
		Color forecolor = {};
		Color backcolor = {};// for next font's, but background, while background use window.color.
		Rectangle window = {};//{TODO}
		byte update_method = 2;// 0: no update, 1: update all, 2: update line
		bool cursor_visible = false;
	protected:
		static void (VideoConsole::* DrawCharPosition_f[])(uni::Point, uni::Color, char);
		static VideoConsole* crt_self;
		friend void _VideoConsoleOut(const char* str, stduint len);
		void DrawCharPosition_8x5(Point disp, Color color, char ch);
		void DrawCharPosition_16x8(Point disp, Color color, char ch);
		void FeedLine();

	public:
		VideoConsole(const VideoControlInterface* vci,
			const Rectangle& win,
			const Color& fore_color = Color::White,
			const Color& back_color = Color::Black) :
			SheetTrait(),
			vci(vci),
			buffer(nullptr), size(0), typ(1), forecolor(fore_color), backcolor(back_color),
			window(win)
		{
			size.x = window.width / (typ ? 8 : 5);
			size.y = window.height / (typ ? 16 : 8);
			window.color = back_color;
		}

		inline void Clear() {
			if (buffer) {
				Color* p0 = buffer;
				for0(y, window.height * window.width) {
					*p0++ = backcolor;
				}
				sheet_parent->Update(this, window);
			}
			else vci->DrawRectangle(window);
		}

		void Start();

		void setModeBuffer(Color* buf) { buffer = buf; }
		void setBackcolor(Color bkcolor) {
			byte buf[4] = { (byte)'\xFE', bkcolor.b, bkcolor.g, bkcolor.r };
			out((rostr)buf, 4);
		}

	public:
		virtual int out(const char* str, stduint len) override;
		virtual int inn() override _TODO;
		virtual void doshow(void*) override;
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override;

	public:

		void curinc();

	// Sheet:
	protected:
		void thisDrawPoint(const Point& disp, Color color);
		void thisDrawRectangle(const Rectangle& rect);
		void thisRollup(stduint height);
	public:
		void RefreshLine();
	public:
		inline auto getCursor() const { return cursor; }
	};

	// Abstract font rendering engine interface
	class FontEngine {
	public:
		virtual ~FontEngine() {}

		// Get width and height of a single character cell in monospaced console
		virtual Size2 GetCellSize() const = 0;

		// Render character into a pixel buffer at (px_x, px_y) with specified pitch
		virtual void DrawChar(
			Color* pixel_buffer,
			stduint pitch_pixels,
			stduint px_x,
			stduint px_y,
			uint32 unicode_char,
			Color fg,
			Color bg
		) const = 0;

		// Retrieve color of a specific sub-pixel (gx, gy) within monospaced cell
		virtual Color GetPixel(
			uint32 unicode_char,
			stduint gx,
			stduint gy,
			Color fg,
			Color bg
		) const = 0;
	};

	// Default bitmap font engine supporting 8x5 and 16x8 formats
	class BitmapFontEngine : public FontEngine {
	protected:
		stduint typ; // 0: 8x5, 1: 16x8
	public:
		BitmapFontEngine(stduint type = 1) : typ(type) {}
		virtual ~BitmapFontEngine() {}

		virtual Size2 GetCellSize() const override {
			return typ ? Size2(8, 16) : Size2(5, 8);
		}
		virtual void DrawChar(
			Color* pixel_buffer,
			stduint pitch_pixels,
			stduint px_x,
			stduint px_y,
			uint32 unicode_char,
			Color fg,
			Color bg
		) const override;
		virtual Color GetPixel(
			uint32 unicode_char,
			stduint gx,
			stduint gy,
			Color fg,
			Color bg
		) const override;
	};

	// One cell of the VideoConsole2 text buffer.
	struct BufferChar {
		Color   back_color = {};    // background color of this cell
		Color   fore_color = {};    // foreground (glyph) color
		uint32  attr = 0;          // attributes: bold/underline/blink/reverse (reserved)
		uint32  unicode_char = 0;  // character code (currently ASCII; lower byte used)
	};

	// VideoConsole2: character-unit console with lazy line-buffer rendering.
	// Follows the same interface and usage pattern as VideoConsole, but maintains
	// a BufferChar text buffer (cols x rows) as primary state instead of pixels.
	//
	// On RollUp: shift text_buf up by one row (cheap), invalidate line_buf.
	// On draw  : rasterize one char-row into line_buf on demand, blit via DrawPoints.
	class VideoConsole2 : public Console_t, public SheetTrait {
	public:
		const VideoControlInterface* vci = nullptr;
	protected:
		Point   cursor  = { 0, 0 };
		Color*  buffer  = nullptr; // pixel buffer (same role as VideoConsole::buffer)
		Size2   size = { 0, 0 };              // char grid dimensions (cols, rows)
		const FontEngine* font_engine = nullptr; // decoupled font engine
	public:
		Color     forecolor = {};
		Color     backcolor = {};
		Rectangle window = {};
		byte      update_method  = 2;    // 0: none, 1: all, 2: line
		bool      cursor_visible = false;
	protected:
		// Character text buffer (authoritative content store): one BufferChar per cell.
		BufferChar* text_buf   = nullptr; // [rows * cols]
		// Pixel cache for one character row: [font_h * cols * font_w].
		Color*  line_buf       = nullptr;
		stdsint line_buf_row   = -1;     // cached row index; -1 = invalid
		bool    line_buf_valid = false;
		stduint cols           = 0;
		stduint rows           = 0;
		// ANSI SGR parser state (persists across out() calls so that sequences
		// like "\x1b[3" + "2m" across two calls are handled correctly).
		//   0 = idle
		//   1 = ESC received, waiting for '['
		//   2 = inside CSI parameter string (collecting digits / ';')
		uint8  esc_state           = 0;
		int    esc_params[8]       = {};  // accumulator for up to 8 numeric params
		int    esc_nparams         = 0;   // index of the param slot being written

	public:
		VideoConsole2(const VideoControlInterface* vci,
			const Rectangle& win,
			const Color& fore_color = Color::White,
			const Color& back_color = Color::Black);

		// Bind font engine dynamically
		void setFontEngine(const FontEngine* fe);

		// Attach external storage.
		// pixel_buf    : pixel buffer (width * height Colors), or nullptr for VCI-direct.
		// text_storage : at least cols * rows BufferChar elements.
		// line_storage : at least (cols * font_w) * font_h Color elements, or nullptr.
		void setBuffers(Color* pixel_buf, BufferChar* text_storage, Color* line_storage);

		// Clear console: blank both buffers, reset cursor.
		void Clear();

		void Start();
		void Stop();

		inline auto getCursor() const { return cursor; }
		inline void setModeBuffer(Color* buf) { buffer = buf; }

		inline stduint getCols() const { return cols; }
		inline stduint getRows() const { return rows; }
		// Number of Color elements required for the line buffer allocation.
		// line_buf must hold one character row: cols * font_w * font_h pixels.
		inline stduint getLineBufferSize() const {
			if (!font_engine) return 0;
			Size2 cell_size = font_engine->GetCellSize();
			return cols * cell_size.x * cell_size.y;
		}

	public:
		virtual int  out(const char* str, stduint len) override;
		virtual int  inn() override _TODO;
		virtual void doshow(void*) override;
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override;
		// Called by LayerManager per-pixel when sheet_buffer == nullptr.
		// Looks up the CharCell in text_buf and returns the fg or bg color.
		// Uses line_buf as a row-level cache when available.
		virtual Color getPoint(Point p) override;

	protected:
		// Invalidate the line buffer (called on RollUp / content change).
		inline void InvalidateLineBuffer() { line_buf_valid = false; }

		// Render char-row `row` into line_buf if not already cached.
		void EnsureLineBuffer(stduint row);

		// Blit line_buf to pixel buffer / VCI at char-row `row`.
		void BlitLineBuffer(stduint row);

		// Scroll up by one char row: shift text buffer, invalidate line cache.
		void thisRollup(stduint height);

		// Advance cursor past bottom, rolling up as needed.
		void FeedLine();

		// Write one BufferChar into text_buf; invalidate line cache for that row.
		void Putchar(stduint cx, stduint cy, BufferChar cell);

		// Advance cursor by one character cell.
		void curinc();

		friend void _VideoConsole2Out(VideoConsole2* crt_self, const char* str, stduint len);
		static VideoConsole2* crt_self2;
	};

	// VideoConsole3: uni::witch::control::TextBox
}
