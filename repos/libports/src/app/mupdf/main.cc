/*
 * \brief  MuPDF for Genode
 * \author Norman Feske
 * \date   2012-01-09
 */

/*
 * Copyright (C) 2012-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <framebuffer_session/connection.h>
#include <base/sleep.h>
#include <util/volatile_object.h>
#include <os/attached_dataspace.h>
#include <os/config.h>
#include <util/geometry.h>
#include <input_session/connection.h>
#include <input/event.h>
#include <input/keycodes.h>
#include <timer_session/connection.h>

/* MuPDF includes */
extern "C" {
#include <fitz.h>
#include <mupdf.h>
#include <muxps.h>
#include <pdfapp.h>
}

/* libc includes */
#include <unistd.h>


/***************
 ** Dithering **
 ***************/

/*
 * XXX blatantly copied from 'demo/src/app/backdrop/main.cc'
 *
 * We should factor-out the dithering support into a separate header file.
 * But where is a good place to put it?
 */

enum { DITHER_SIZE = 16, DITHER_MASK = DITHER_SIZE - 1 };

static const int dither_matrix[DITHER_SIZE][DITHER_SIZE] = {
  {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
  { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
  {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
  { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
  {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
  { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
  {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
  { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
  {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
  { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
  {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
  { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
  {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
  { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
  {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
  { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
};


static inline uint16_t rgb565(int r, int g, int b)
{
	enum {
		R_MASK = 0xf800, R_LSHIFT = 8,
		G_MASK = 0x07e0, G_LSHIFT = 3,
		B_MASK = 0x001f, B_RSHIFT = 3
	};
	return ((r << R_LSHIFT) & R_MASK)
	     | ((g << G_LSHIFT) & G_MASK)
	     | ((b >> B_RSHIFT) & B_MASK);
}


static void convert_line_rgba_to_rgb565(const unsigned char *rgba_src,
                                        uint16_t *dst, int num_pixels, int line)
{
	using namespace Genode;

	enum { CHANNEL_MAX = 255 };

	int const *dm = dither_matrix[line & DITHER_MASK];

	for (int i = 0; i < num_pixels; i++) {
		int v = dm[i & DITHER_MASK] >> 5;

		*dst++ = rgb565(min(v + (int)rgba_src[0], (int)CHANNEL_MAX),
		                min(v + (int)rgba_src[1], (int)CHANNEL_MAX),
		                min(v + (int)rgba_src[2], (int)CHANNEL_MAX));

		/* we ignore the alpha channel */

		rgba_src += 4; /* next pixel */
	}
}


/**************
 ** PDF view **
 **************/

class Pdf_view
{
	public:

		/**
		 * Exception types
		 */
		class Non_supported_framebuffer_mode { };
		class Invalid_input_file_name { };
		class Unexpected_document_color_depth { };

	private:

		struct _Framebuffer : Framebuffer::Connection
		{
			typedef uint16_t pixel_t;

			Framebuffer::Mode mode;

			Genode::Lazy_volatile_object<Genode::Attached_dataspace> ds;

			void handle_resize()
			{
				mode = Framebuffer::Connection::mode();
				if (mode.format() != Framebuffer::Mode::RGB565) {
					PERR("Color modes other than RGB565 are not supported. Exiting.");
					throw Non_supported_framebuffer_mode();
				}
				PINF("Framebuffer is %dx%d\n", mode.width(), mode.height());

				ds.construct(Framebuffer::Connection::dataspace());
			}

			_Framebuffer() { handle_resize(); }

			pixel_t *base() { return ds->local_addr<pixel_t>(); }

		} _framebuffer;

		void _update_pdfapp_parameters()
		{
			_pdfapp.scrw = _framebuffer.mode.width();
			_pdfapp.scrh = _framebuffer.mode.height();

			/*
			 * XXX replace heuristics with a meaningful computation
			 *
			 * The magic values are hand-tweaked manually to accommodating the
			 * use case of showing slides.
			 */
			_pdfapp.resolution = Genode::min(_framebuffer.mode.width()/5,
			                                 _framebuffer.mode.height()/3.8);
		}

		void _handle_resize(unsigned)
		{
			_framebuffer.handle_resize();

			_update_pdfapp_parameters();

			/* reload file */
			pdfapp_onkey(&_pdfapp, 'r');
		}

		Genode::Signal_dispatcher<Pdf_view> _resize_dispatcher;

		pdfapp_t _pdfapp;

		Input::Connection _input;

		Genode::Attached_dataspace _input_ds { _input.dataspace() };

		int _key_cnt = 0;

		void _handle_input(unsigned);

		Genode::Signal_dispatcher<Pdf_view> _input_dispatcher;

	public:

		/**
		 * Constructor
		 *
		 * \throw Non_supported_framebuffer_mode
		 * \throw Invalid_input_file_name
		 * \throw Unexpected_document_color_depth
		 */
		Pdf_view(Genode::Signal_receiver &sig_rec, char const *file_name)
		:
			_resize_dispatcher(sig_rec, *this, &Pdf_view::_handle_resize),
			_input_dispatcher(sig_rec, *this, &Pdf_view::_handle_input)
		{
			_input.sigh(_input_dispatcher);
			_framebuffer.mode_sigh(_resize_dispatcher);

			pdfapp_init(&_pdfapp);
			_pdfapp.userdata = this;
			_update_pdfapp_parameters();
			_pdfapp.pageno     = 0;    /* XXX read from config */

			int fd = open(file_name, O_BINARY | O_RDONLY, 0666);
			if (fd < 0) {
				PERR("Could not open input file \"%s\", Exiting.", file_name);
				throw Invalid_input_file_name();
			}
			pdfapp_open(&_pdfapp, (char *)file_name, fd, 0);

			if (_pdfapp.image->n != 4) {
				PERR("Unexpected color depth, expected 4, got %d, Exiting.",
				     _pdfapp.image->n);
				throw Unexpected_document_color_depth();
			}
		}

		void show();

		void handle_key(int ascii)
		{
			pdfapp_onkey(&_pdfapp, ascii);
		}
};


void Pdf_view::show()
{
	Genode::Area<> const fb_size(_framebuffer.mode.width(), _framebuffer.mode.height());
	int const x_max = Genode::min((int)fb_size.w(), _pdfapp.image->w);
	int const y_max = Genode::min((int)fb_size.h(), _pdfapp.image->h);

	/* clear framebuffer */
	memset(_framebuffer.base(), 0, sizeof(_Framebuffer::pixel_t)*fb_size.count());

	Genode::size_t src_line_bytes   = _pdfapp.image->n * _pdfapp.image->w;
	unsigned char *src_line         = _pdfapp.image->samples;

	Genode::size_t dst_line_width   = fb_size.w(); /* in pixels */
	_Framebuffer::pixel_t *dst_line = _framebuffer.base();

	/* skip first two lines as they contain white (XXX) */
	src_line += 2*src_line_bytes;
	dst_line += 2*dst_line_width;
	int const tweaked_y_max = y_max - 2;

	/* center vertically if the dst buffer is higher than the image */
	if (_pdfapp.image->h < (int)fb_size.h())
		dst_line += dst_line_width*((fb_size.h() - _pdfapp.image->h)/2);

	/* center horizontally if the dst buffer is wider than the image */
	if (_pdfapp.image->w < (int)fb_size.w())
		dst_line += (fb_size.w() - _pdfapp.image->w)/2;

	for (int y = 0; y < tweaked_y_max; y++) {
		convert_line_rgba_to_rgb565(src_line, dst_line, x_max, y);
		src_line += src_line_bytes;
		dst_line += dst_line_width;
	}

	_framebuffer.refresh(0, 0, _framebuffer.mode.width(), _framebuffer.mode.height());
}


extern "C" void _sigprocmask()
{
	/* suppress debug message by default "not-implemented" implementation */
}


/**************************
 ** Called from pdfapp.c **
 **************************/

void winrepaint(pdfapp_t *pdfapp)
{
	PDBG("called");
	Pdf_view *pdf_view = (Pdf_view *)pdfapp->userdata;
	pdf_view->show();
}


void winrepaintsearch(pdfapp_t *)
{
	PDBG("not implemented");
}


void wincursor(pdfapp_t *, int curs)
{
}


void winerror(pdfapp_t *, fz_error error)
{
	PDBG("error=%d", error);
	Genode::sleep_forever();
}


void winwarn(pdfapp_t *, char *msg)
{
	PWRN("MuPDF: %s", msg);
}


void winhelp(pdfapp_t *)
{
	PDBG("not implemented");
}


char *winpassword(pdfapp_t *, char *)
{
	PDBG("not implemented");
	return NULL;
}


void winclose(pdfapp_t *app)
{
	PDBG("not implemented");
}


void winreloadfile(pdfapp_t *)
{
	PDBG("not implemented");
}


void wintitle(pdfapp_t *app, char *s)
{
}


void winresize(pdfapp_t *app, int w, int h)
{
}


/******************
 ** Main program **
 ******************/

static int keycode_to_ascii(int code)
{
	switch (code) {
	case Input::KEY_LEFT:      return 'h';
	case Input::KEY_RIGHT:     return 'l';
	case Input::KEY_DOWN:      return 'j';
	case Input::KEY_UP:        return 'k';
	case Input::KEY_PAGEDOWN:
	case Input::KEY_ENTER:     return ' ';
	case Input::KEY_PAGEUP:
	case Input::KEY_BACKSPACE: return 'b';
	case Input::KEY_9:         return '-';
	case Input::KEY_0:         return '+';
	default:                   return 0;
	}
}


void Pdf_view::_handle_input(unsigned)
{
	Input::Event const *ev_buf = _input_ds.local_addr<Input::Event>();

	for (int i = 0, num_ev = _input.flush(); i < num_ev; i++) {

		Input::Event const &ev = ev_buf[i];

		if (ev.type() == Input::Event::PRESS)   _key_cnt++;
		if (ev.type() == Input::Event::RELEASE) _key_cnt--;

		if (ev.type() == Input::Event::PRESS && _key_cnt == 1) {

			PDBG("key %d pressed", ev.code());

			int const ascii = keycode_to_ascii(ev.code());
			if (ascii)
				handle_key(ascii);
		}
	}
}


int main(int, char **)
{
	/* read name of PDF file from config */
	char pdf[64];
	pdf[0] = 0;
	try {
		Genode::config()->xml_node().attribute("pdf").value(pdf, sizeof(pdf));
	} catch (...) {
		PDBG("config attribute 'pdf' missing");
		return -1;
	}

	static Genode::Signal_receiver sig_rec;
	static Pdf_view pdf_view(sig_rec, pdf);

	while (1) {

		Genode::Signal signal = sig_rec.wait_for_signal();

		Genode::Signal_dispatcher_base *dispatcher =
			static_cast<Genode::Signal_dispatcher_base *>(signal.context());

		dispatcher->dispatch(signal.num());
	}

	return 0;
}
