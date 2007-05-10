/*
	NatFeat VDI driver, software

	ARAnyM (C) 2001-2007 Standa Opichal and others, see the AUTHORS file

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "host.h"
#include "hostscreen.h"
#include "nfvdi.h"
#include "nfvdi_soft.h"

#define DEBUG 0
#include "debug.h"

#include <cstring>

/*--- Defines ---*/

#define EINVFN -32

static const uint8 vdi_colours[] = { 0,2,3,6,4,7,5,8,9,10,11,14,12,15,13,255 };
static const uint8 tos_colours[] = { 0,255,1,2,4,6,3,5,7,8,9,10,12,14,11,13 };
#define toTosColors( color ) \
    ( (color)<(sizeof(tos_colours)/sizeof(*tos_colours)) ? tos_colours[color] : ((color) == 255 ? 15 : (color)) )
#define toVdiColors( color ) \
    ( (color)<(sizeof(vdi_colours)/sizeof(*vdi_colours)) ? vdi_colours[color] : color)

/*--- Types ---*/

/*--- Variables ---*/

/*--- Public functions ---*/

SoftVdiDriver::SoftVdiDriver()
{
}

SoftVdiDriver::~SoftVdiDriver()
{
}

void SoftVdiDriver::reset(void)
{
	VdiDriver::reset();
}

/*--- Private functions ---*/

int32 SoftVdiDriver::openWorkstation(void)
{
	return VdiDriver::openWorkstation();
}

int32 SoftVdiDriver::closeWorkstation(void)
{
	return VdiDriver::closeWorkstation();
}


/**
 * Get a coloured pixel.
 *
 * c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y)
 * read_pixel
 * In:  a1  VDI struct, source MFDB
 *  d1  x
 *  d2  y
 *
 * Only one mode here.
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB.
 *
 * Since an MFDB is passed, the source is not necessarily the screen.
 **/

int32 SoftVdiDriver::getPixel(memptr vwk, memptr src, int32 x, int32 y)
{
	uint32 color = 0;

	if (src)
		return VdiDriver::getPixel(vwk, src, x, y);

	if (!hostScreen.renderBegin())
		return 0;
	color = hostScreen.getPixel((int16)x, (int16)y);
	hostScreen.renderEnd();

	return color;
}

/**
 * Set a coloured pixel.
 *
 * c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour)
 * write_pixel
 * In:   a1  VDI struct, destination MFDB
 *   d0  colour
 *   d1  x or table address
 *   d2  y or table length (high) and type (low)
 * XXX: ?
 *
 * This function has two modes:
 *   - single pixel
 *   - table based multi pixel (special mode 0 (low word of 'y'))
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB.
 *
 * As usual, only the first one is necessary, and a return with d0 = -1
 * signifies that a special mode should be broken down to the basic one.
 *
 * Since an MFDB is passed, the destination is not necessarily the screen.
 **/
 
int32 SoftVdiDriver::putPixel(memptr vwk, memptr dst, int32 x, int32 y,
	uint32 color)
{
	if (vwk & 1)
		return 0;

	if (dst)
		return VdiDriver::putPixel(vwk, dst, x, y, color);

	// To screen
	if (!hostScreen.renderBegin())
		return 1;
	hostScreen.putPixel((int16)x, (int16)y, color);
	hostScreen.renderEnd();
	hostScreen.update((int16)x, (int16)y, 1, 1, true);

	D(bug("softvdi: putpixel(x,y,0x%08x)", x,y,color));
	return 1;
}

/**
 * Draw the mouse
 *
 * Draw a coloured line between two points.
 *
 * c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse)
 * mouse_draw
 * In:  a1  Pointer to Workstation struct
 *  d0/d1   x,y
 *  d2  0 - move shown  1 - move hidden  2 - hide  3 - show  >7 - change shape (pointer to mouse struct)
 *
 * Unlike all the other functions, this does not receive a pointer to a VDI
 * struct, but rather one to the screen's workstation struct. This is
 * because the mouse handling concerns the screen as a whole (and the
 * routine is also called from inside interrupt routines).
 *
 * The Mouse structure pointer doubles as a mode variable. If it is a small
 * number, the mouse's state is supposed to change somehow, while a large
 * number is a pointer to a new mouse shape.
 *
 * This is currently not a required function, but it probably should be.
 * The fallback handling is not done in the usual way, and to make it
 * at least somewhat usable, the mouse pointer is reduced to 4x4 pixels.
 *
 * typedef struct Fgbg_ {
 *   short background;
 *   short foreground;
 * } Fgbg;
 *
 * typedef struct Mouse_ {
 * 0  short type;
 * 2   short hide;
 *   struct position_ {
 * 4   short x;
 * 6   short y;
 *   } position;
 *   struct hotspot_ {
 * 8   short x;
 * 10   short y;
 *   } hotspot;
 * 12  Fgbg colour;
 * 16  short mask[16];
 * 48  short data[16];
 * 80  void *extra_info;
 * } Mouse;
 **/

extern "C" {
	static uint16 reverse_bits(uint16 data)
	{
		uint16 res = 0;
		for(uint16 i = 0; i <= 15; i++)
			res |= ((data >> i) & 1) << (15 - i);
		return res;
	}
}

void SoftVdiDriver::restoreMouseBackground(void)
{
	int16 x = Mouse.storage.x;
	int16 y = Mouse.storage.y;

	if (!hostScreen.renderBegin())
		return;

	for(uint16 i = 0; i < Mouse.storage.height; i++)
		for(uint16 j = 0; j < Mouse.storage.width; j++)
			hostScreen.putPixel(x + j, y + i, Mouse.storage.background[i][j]);

	hostScreen.renderEnd();
	hostScreen.update(x, y, Mouse.storage.width, Mouse.storage.height, true);
}

void SoftVdiDriver::saveMouseBackground(int16 x, int16 y, int16 width,
	int16 height)
{
	D2(bug("fVDI: saveMouseBackground: %d,%d,%d,%d", x, y, width, height));

	for(uint16 i = 0; i < height; i++)
		for(uint16 j = 0; j < width; j++) {
			Mouse.storage.background[i][j] = hostScreen.getPixel(x + j, y + i);
		}

	Mouse.storage.x = x;
	Mouse.storage.y = y;
	Mouse.storage.height = height;
	Mouse.storage.width = width;
}

int SoftVdiDriver::drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
	uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor, uint32 bgColor,
	uint32 mouse_type)
{
	if (bx_options.nfvdi.use_host_mouse_cursor) {
		return VdiDriver::drawMouse(wk,x,y,mode,data,hot_x,hot_y,fgColor,bgColor,mouse_type);
	}

	DUNUSED(wk);
	DUNUSED(mouse_type);
	D2(bug("fVDI: mouse mode: %x", mode));

	switch (mode) {
	case 4:
	case 0:  // move shown
		restoreMouseBackground();
		break;
	case 5:
	case 1:  // move hidden
		return 1;
	case 2:  // hide
		restoreMouseBackground();
		return 1;
	case 3:  // show
		break;
	case 6:
	case 7:
		break;

	default: // change pointer shape
		D(bug("fVDI: mouse : %08x, %08x, (%08x,%08x)", hot_x, hot_y, fgColor, bgColor));
		memptr fPatterAddress = (memptr)mode;
		for(uint16 i = 0; i < 32; i += 2)
			Mouse.mask[i >> 1] = reverse_bits(ReadInt16(fPatterAddress + i));
		fPatterAddress  = (memptr)data;
		for(uint16 i = 0; i < 32; i += 2)
			Mouse.shape[i >> 1] = reverse_bits(ReadInt16(fPatterAddress + i));

		Mouse.hotspot.x = hot_x & 0xf;
		Mouse.hotspot.y = hot_y & 0xf;
		Mouse.storage.color.foreground = fgColor;
		Mouse.storage.color.background = bgColor;

		return 1;
	}

	// handle the mouse hotspot point
	x -= Mouse.hotspot.x;
	y -= Mouse.hotspot.y;

	// roll the pattern properly
	uint16 mm[16];
	uint16 md[16];
	uint16 shift = x & 0xf;
	for(uint16 i = 0; i <= 15; i++) {
		mm[(i + (y & 0xf)) & 0xf] = (Mouse.mask[i]  << shift) | ((Mouse.mask[i]  >> (16 - shift)) & ((1 << shift) - 1));
		md[(i + (y & 0xf)) & 0xf] = (Mouse.shape[i] << shift) | ((Mouse.shape[i] >> (16 - shift)) & ((1 << shift) - 1));
	}

	// beware of the edges of the screen
	int16 w, h;

	if (x < 0) {
		w = 16 + x;
		x = 0;
	} else {
		w = 16;
		if ((int16)x + 16 >= (int32)hostScreen.getWidth())
			w = hostScreen.getWidth() - (int16)x;
	}
	if (y < 0) {
		h = 16 + y;
		y = 0;
	} else {
		h = 16;
		if ((int16)y + 16 >= (int32)hostScreen.getHeight())
			h = hostScreen.getHeight() - (int16)y;
	}

	D2(bug("fVDI: mouse x,y: %d,%d,%d,%d (%x,%x)", x, y, w, h,
	   Mouse.storage.color.background, Mouse.storage.color.foreground));

	// draw the mouse
	if (!hostScreen.renderBegin())
		return 1;

	saveMouseBackground(x, y, w, h);

	hostScreen.fillArea(x, y, w, h, mm, Mouse.storage.color.background);
	hostScreen.fillArea(x, y, w, h, md, Mouse.storage.color.foreground);

	hostScreen.renderEnd();
	hostScreen.update((uint16)x, (uint16)y, w, h, true);

	return 1;
}

/**
 * Expand a monochrome area to a coloured one.
 *
 * c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
 *                             MFDB *dst, long dst_x, long dst_y,
 *                             long w, long h, long operation, long colour)
 * expand_area
 * In:  a1  VDI struct, destination MFDB, VDI struct, source MFDB
 *  d0  height and width to move (high and low word)
 *  d1-d2   source coordinates
 *  d3-d4   destination coordinates
 *  d6  background and foreground colour
 *  d7  logic operation
 *
 * Only one mode here.
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB, and then comes a VDI struct
 * pointer again (the same) and a pointer to the source MFDB.
 *
 * Since MFDBs are passed, the screen is not necessarily involved.
 *
 * A return with 0 gives a fallback (normally pixel by pixel drawing by
 * the fVDI engine).
 *
 * typedef struct MFDB_ {
 *   short *address;
 *   short width;
 *   short height;
 *   short wdwidth;
 *   short standard;
 *   short bitplanes;
 *   short reserved[3];
 * } MFDB;
 **/

int32 SoftVdiDriver::expandArea(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp,
	uint32 fgColor, uint32 bgColor)
{
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	uint16 pitch = ReadInt16(src + MFDB_WDWIDTH) * 2; // the byte width (always monochrom);
	/* Linux does not wrap on words */
	if ( (uint32)ReadInt16( src + MFDB_STAND ) & 0x1000 ) {
		pitch >>= 1;
	}

	memptr data  = ReadInt32(src + MFDB_ADDRESS) + sy * pitch; // MFDB *src->address;

	D(bug("fVDI: %s %x %d,%d:%d,%d:%d,%d (%lx, %lx)", "expandArea", logOp, sx, sy, dx, dy, w, h, fgColor, bgColor ));
	D2(bug("fVDI: %s %x,%x : %x,%x", "expandArea - MFDB addresses", src, dest, ReadInt32( src ),ReadInt32( dest )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - src: data address, MFDB wdwidth << 1, bitplanes", data, pitch, ReadInt16( src + MFDB_NPLANES )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - dst: data address, MFDB wdwidth << 1, bitplanes", ReadInt32(dest), ReadInt16(dest + MFDB_WDWIDTH) * (ReadInt16(dest + MFDB_NPLANES) >> 2), ReadInt16(dest + MFDB_NPLANES)));

	if (dest) {
		return VdiDriver::expandArea(vwk, src, sx, sy, dest, dx, dy, w, h, logOp, fgColor, bgColor);
	}

	if ( (uint32)ReadInt16( src + MFDB_STAND ) & 0x100 ) {
		if ( ReadInt16( src + MFDB_NPLANES ) != 8 ) {
			bug("fVDI: Only 8bit chunky expand is supported so far.");
			return 0;
		}

		/* FIXME! This is a hack as this class should not use SDL functions directly */

		/* 8bit greyscale aplha expand */
		SDL_Surface *screen = hostScreen.getPhysicalSurface();
		if ( screen->format->BytesPerPixel != 4 ) {
			bug("fVDI: Only 4byte SDL_Surface color depths supported so far.");
			return 0;
		}

		SDL_Surface *asurf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
				screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0xFF000000);
		if ( asurf == NULL ) return 0;

		if (logOp == 1) {
			D(bug("fVDI: expandArea 8bit: logOp=%d screen=%p, format=%p [%d]", logOp, screen, screen->format, screen->format->BytesPerPixel));

			/* no alpha surface */
			SDL_Surface *blocksurf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, screen->format->BitsPerPixel,
					screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0xFF000000);
			if ( blocksurf == NULL ) return 0;

			/* fill with the background color */
			for(uint16 j = 0; j < h; j++) {
				uint32 *dst = (uint32 *)blocksurf->pixels + j * blocksurf->pitch/4;
				for(uint16 i = 0; i < w; i++) *dst++ = bgColor | 0xff000000UL;
			}

			/* transparent alpha blit */
			for(uint16 j = 0; j < h; j++) {
				uint32 *dst = (uint32 *)asurf->pixels + j * asurf->pitch/4;
				for(uint16 i = sx; i < sx + w; i++) {
					*dst++ = fgColor | (ReadInt8(data + j * pitch + i) << 24);
				}
			}
			SDL_Rect identRect = { 0, 0, w, h };
			SDL_BlitSurface(asurf,NULL,blocksurf,&identRect);
			SDL_FreeSurface(asurf);

			/* blit the whole thing to the screen */
			asurf = blocksurf;
		} else {
			D(bug("fVDI: expandArea 8bit: logOp=%d %lx %lx", logOp, fgColor, bgColor));

			for(uint16 j = 0; j < h; j++) {
				uint32 *dst = (uint32 *)asurf->pixels + j * asurf->pitch/4;
				switch(logOp) {
					case 2:
						for(uint16 i = sx; i < sx + w; i++) {
							*dst++ = fgColor | (ReadInt8(data + j * pitch + i) << 24);
						}
						break;
					case 3:
						for(uint16 i = sx; i < sx + w; i++) {
							*dst++ = (~hostScreen.getPixel(dx + i - sx, dy + j) & ~0xff000000UL) | (ReadInt8(data + j * pitch + i) << 24);
						}
						break;
					case 4:
						for(uint16 i = sx; i < sx + w; i++) {
							*dst++ = bgColor | ((0xff - ReadInt8(data + j * pitch + i)) << 24);
						}
						break;
				}
			}
		}

		D(bug("fVDI: %s %x, %d, %d", "8BIT expandArea - src: data address, MFDB wdwidth << 1, bitplanes", data, pitch, ReadInt16( src + MFDB_NPLANES )));

		SDL_Rect destRect = { dx, dy, w, h };
		SDL_BlitSurface(asurf,NULL,screen,&destRect);
		SDL_FreeSurface(asurf);

		hostScreen.update(dx, dy, w, h, true);
		return 1;
	}

	if (!hostScreen.renderBegin())
		return 1;

	D(bug("fVDI: expandArea M->S"));
	for(uint16 j = 0; j < h; j++) {
		D2(fprintf(stderr, "fVDI: bmp:"));

		uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
		for(uint16 i = sx; i < sx + w; i++) {
			if (i % 16 == 0)
				theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

			D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
			switch(logOp) {
				case 1:
					hostScreen.putPixel(dx + i - sx, dy + j, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
					break;
				case 2:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						hostScreen.putPixel(dx + i - sx, dy + j, fgColor);
					break;
				case 3:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						hostScreen.putPixel(dx + i - sx, dy + j, ~hostScreen.getPixel(dx + i - sx, dy + j));
					break;
				case 4:
					if (!((theWord >> (15 - (i & 0xf))) & 1))
						hostScreen.putPixel(dx + i - sx, dy + j, bgColor);
					break;
			}
		}
		D2(bug("")); //newline
	}

	hostScreen.renderEnd();
	hostScreen.update(dx, dy, w, h, true);

	return 1;
}

/**
 * Fill a coloured area using a monochrome pattern.
 *
 * c_fill_area(Virtual *vwk, long x, long y, long w, long h,
 *                           short *pattern, long colour, long mode, long interior_style)
 * fill_area
 * In:  a1  VDI struct
 *  d0  height and width to fill (high and low word)
 *  d1  x or table address
 *  d2  y or table length (high) and type (low)
 *  d3  pattern address
 *  d4  colour
 *
 * This function has two modes:
 * - single block to fill
 * - table based y/x1/x2 spans to fill (special mode 0 (low word of 'y'))
 *
 * As usual, only the first one is necessary, and a return with d0 = -1
 * signifies that a special mode should be broken down to the basic one.
 *
 * An immediate return with 0 gives a fallback (normally line based drawing
 * by the fVDI engine for solid fills, otherwise pixel by pixel).
 * A negative return will break down the special mode into separate calls,
 * with no more fallback possible.
 **/

int32 SoftVdiDriver::fillArea(memptr vwk, uint32 x_, uint32 y_, int32 w,
	int32 h, memptr pattern_addr, uint32 fgColor, uint32 bgColor,
	uint32 logOp, uint32 interior_style)
{
	DUNUSED(interior_style);
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	uint16 pattern[16];
	for(int i = 0; i < 16; ++i)
		pattern[i] = ReadInt16(pattern_addr + i * 2);

	memptr table = 0;

	int x = x_;
	int y = y_;

	if ((long)vwk & 1) {
		if ((y_ & 0xffff) != 0)
			return -1;		// Don't know about this kind of table operation
		table = (memptr)x_;
		h = (y_ >> 16) & 0xffff;
		vwk -= 1;
	}

	D(bug("fVDI: %s %d %d,%d:%d,%d : %d,%d p:%x, (fgc:%lx : bgc:%lx)", "fillArea",
	      logOp, x, y, w, h, x + w - 1, x + h - 1, *pattern,
	      fgColor, bgColor));

	if (!hostScreen.renderBegin())
		return 1;

	int minx = 1000000;
	int miny = 1000000;
	int maxx = -1000000;
	int maxy = -1000000;

	/* Perform rectangle fill. */
	if (!table) {
		hostScreen.fillArea(x, y, w, h, pattern, fgColor, bgColor, logOp);
		minx = x;
		miny = y;
		maxx = x + w - 1;
		maxy = y + h - 1;
	} else {
		for(h = h - 1; h >= 0; h--) {
			y = (int16)ReadInt16(table); table+=2;
			x = (int16)ReadInt16(table); table+=2;
			w = (int16)ReadInt16(table) - x + 1; table+=2;
			hostScreen.fillArea(x, y, w, 1, pattern, fgColor, bgColor, logOp);
			if (x < minx)
				minx = x;
			if (y < miny)
				miny = y;
			if (x + w - 1 > maxx)
				maxx = x + w - 1;
			if (y > maxy)
				maxy = y;
		}
	}

	hostScreen.renderEnd();
	hostScreen.update(minx, miny, maxx - minx + 1, maxy - miny + 1, true);

	return 1;
}

void SoftVdiDriver::fillArea(uint32 x, uint32 y, uint32 w, uint32 h,
                             uint16* pattern, uint32 fgColor, uint32 bgColor,
                             uint32 logOp)
{
	if (!hostScreen.renderBegin())
		return;

	hostScreen.fillArea(x, y, w, h, pattern, fgColor, bgColor, logOp);

	hostScreen.renderEnd();
	hostScreen.update(x, y, w, h, true);
}

/**
 * Blit an area
 *
 * c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
 *                        MFDB *dst, long dst_x, long dst_y,
 *                        long w, long h, long operation)
 * blit_area
 * In:  a1  VDI struct, destination MFDB, VDI struct, source MFDB
 *  d0  height and width to move (high and low word)
 *  d1-d2   source coordinates
 *  d3-d4   destination coordinates
 *  d7  logic operation
 *
 * Only one mode here.
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB, and then comes a VDI struct
 * pointer again (the same) and a pointer to the source MFDB.
 *
 * Since MFDBs are passed, the screen is not necessarily involved.
 *
 * A return with 0 gives a fallback (normally pixel by pixel drawing by the
 * fVDI engine).
 **/

int32 SoftVdiDriver::blitArea_M2S(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);
	DUNUSED(dest);

	if (!hostScreen.renderBegin())
		return 1;

	uint32 planes = ReadInt16(src + MFDB_NPLANES);			// MFDB *src->bitplanes
	uint32 pitch  = ReadInt16(src + MFDB_WDWIDTH) * planes * 2;	// MFDB *src->pitch
	if ( (uint32)ReadInt16( src + MFDB_STAND ) & 0x1000 ) {
		/* Linux does not wrap on words */
		pitch >>= 1;
	}
	memptr data   = ReadInt32(src) + sy * pitch;			// MFDB *src->address host OS address

	uint32 srcData;
	uint32 destData;

	switch(planes) {
		case 16:
			if (logOp != 3) {
				for(int32 j = 0; j < h; j++) {
					for(int32 i = sx; i < sx + w; i++) {
						srcData = ReadInt16(data + j * pitch + i * 2);
						destData = hostScreen.getPixel(dx + i - sx, dy + j);
						destData = applyBlitLogOperation(logOp, destData, srcData);
						hostScreen.putPixel(dx + i - sx, dy + j, destData);
					}
				}
			} else {
				uint16* daddr_base = (uint16*)((long)hostScreen.getVideoramAddress() +
				                          dy * hostScreen.getPitch() + dx * 2);
				memptr saddr_base = data + sx * 2;
				for(int32 j = 0; j < h; j++) {
					uint16* daddr = daddr_base;
					memptr saddr = saddr_base;
					daddr_base += hostScreen.getPitch() / 2;
					saddr_base += pitch;
					for(int32 i = 0; i < w; i++) {
						destData = ReadInt16(saddr);
						saddr += 2;
						*daddr++ = destData;
					}
				}
			}
			break;
		case 24:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					srcData = get_dtriplet(data + j * pitch + i * 3);
					destData = hostScreen.getPixel(dx + i - sx, dy + j);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					hostScreen.putPixel(dx + i - sx, dy + j, destData);
				}
			break;
		case 32:
			if (logOp != 3) {
				for(int32 j = 0; j < h; j++)
					for(int32 i = sx; i < sx + w; i++) {
						srcData = ReadInt32(data + j * pitch + i * 4);
						destData = hostScreen.getPixel(dx + i - sx, dy + j);
						destData = applyBlitLogOperation(logOp, destData, srcData);
						hostScreen.putPixel(dx + i - sx, dy + j, destData);
					}
			} else {
				uint32* daddr_base = (uint32*)((long)hostScreen.getVideoramAddress() +
				                          dy * hostScreen.getPitch() + dx * 4);
				memptr saddr_base = data + sx * 4;
				for(int32 j = 0; j < h; j++) {
					uint32* daddr = daddr_base;
					memptr saddr = saddr_base;
					daddr_base += hostScreen.getPitch() / 4;
					saddr_base += pitch;
					for(int32 i = 0; i < w; i++) {
						destData = ReadInt32(saddr);
						saddr += 4;
						*daddr++ = destData;
					}
				}
			}
			break;

		default: // bitplane modes...
			if (planes < 16) {
				if ( ((uint32)ReadInt16( src + MFDB_STAND ) & 0x100) && planes == 8) {
					D(bug("fVDI: blitArea M->S: chunky8bit"));
					for(int32 j = 0; j < h; j++) {
						for(int32 i = sx; i < sx + w; i++) {
							srcData = ReadInt8(data + j * pitch + i);
							destData = hostScreen.getPixel(dx + i - sx, dy + j);
							destData = applyBlitLogOperation(logOp, destData, srcData);
							hostScreen.putPixel(dx + i - sx, dy + j, destData);
						}
					}
				} else {
					uint8 color[16];

					D(bug("fVDI: blitArea M->S: bitplaneToChunky conversion"));
					uint16 *dataHost = (uint16*)Atari2HostAddr(data);
					// FIXME: Hack! Should use the get_X() methods above

					for(int32 j = 0; j < h; j++) {
						uint32 wordIndex = (j * pitch >> 1) + (sx >> 4) * planes;
						hostScreen.bitplaneToChunky(&dataHost[wordIndex], planes, color);

						for(int32 i = sx; i < sx + w; i++) {
							uint8 bitNo = i & 0xf;
							if (bitNo == 0) {
								uint32 wordIndex = (j * pitch >> 1) + (i >> 4) * planes;
								hostScreen.bitplaneToChunky(&dataHost[wordIndex], planes, color);
							}

							destData = hostScreen.getPixel(dx + i - sx, dy + j);
							destData = applyBlitLogOperation(logOp, destData, color[bitNo]);
							hostScreen.putPixel(dx + i - sx, dy + j, destData);
						}
					}
				}
			}
	}

	hostScreen.renderEnd();
	hostScreen.update(dx, dy, w, h, true);

	return 1;
}

int32 SoftVdiDriver::blitArea_S2M(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);
	DUNUSED(src);
	DUNUSED(dest);

	if (!hostScreen.renderBegin())
		return 1;

	uint32 planes = ReadInt16(dest + MFDB_NPLANES);			// MFDB *dest->bitplanes
	uint32 destPitch = ReadInt16(dest + MFDB_WDWIDTH) * planes * 2;	// MFDB *dest->pitch
	memptr destAddress = ReadInt32(dest);

	uint32 srcData;
	uint32 destData;

	switch(planes) {
		case 16:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 2 + (dy + j) * destPitch;
					srcData = hostScreen.getPixel(i, sy + j);
					destData = ReadInt16(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					WriteInt16(destAddress + offset, destData);
				}
			break;
		case 24:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 3 + (dy + j) * destPitch;
					srcData = hostScreen.getPixel( i, sy + j );
					destData = get_dtriplet(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					put_dtriplet(destAddress + offset, destData);
				}
			break;
		case 32:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 4 + (dy + j) * destPitch;
					srcData = hostScreen.getPixel(i, sy + j);
					destData = ReadInt32(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					WriteInt32(destAddress + offset, destData);
				}
			break;

		default:
			if (planes < 16) {
				D(bug("fVDI: blitArea S->M: bitplane conversion"));

				uint16 bitplanePixels[8];
				uint32 pitch = hostScreen.getPitch();
				uint8* dataHost = (uint8*)hostScreen.getVideoramAddress() + sy * pitch;

				for(int32 j = 0; j < h; j++) {
					uint32 pixelPosition = j * pitch + sx & ~0xf; // div 16
					chunkyToBitplane(dataHost + pixelPosition, planes, bitplanePixels);
					for(uint32 d = 0; d < planes; d++)
						WriteInt16(destAddress + (((dx >> 4) * planes) + d) * 2 + (dy + j) * destPitch, bitplanePixels[d]);

					for(int32 i = sx; i < sx + w; i++) {
						uint8 bitNo = i & 0xf;
						if (bitNo == 0) {
							uint32 wordIndex = ((dx + i - sx) >> 4) * planes;
							uint32 pixelPosition = j * pitch + i & ~0xf; // div 16
							chunkyToBitplane(dataHost + pixelPosition, planes, bitplanePixels);
							for(uint32 d = 0; d < planes; d++)
								WriteInt16(destAddress + (wordIndex + d) * 2 + (dy + j) * destPitch, bitplanePixels[d]);
						}
					}
				}
			}
	}

	hostScreen.renderEnd();
	return 1;
}

int32 SoftVdiDriver::blitArea_S2S(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);
	DUNUSED(src);
	DUNUSED(dest);

	if (logOp == 3) {
	        // for S->S blits... -> SDL does the whole thing at once

		// if (!hostScreen.renderBegin()) // the surface must _not_ be locked for blitArea (SDL_BlitSurface)
		hostScreen.blitArea(sx, sy, dx, dy, w, h);
	} else {
		if (!hostScreen.renderBegin())
			return 1;

		uint32 srcData;
		uint32 destData;

		for(int32 j = 0; j < h; j++)
			for(int32 i = 0; i < w; i++) {
				srcData = hostScreen.getPixel(i + sx, sy + j);
				destData = hostScreen.getPixel(i + dx, dy + j);
				destData = applyBlitLogOperation(logOp, destData, srcData);
				hostScreen.putPixel(dx + i, dy + j, destData);
			}

		hostScreen.renderEnd();
	}

	hostScreen.update(sx, sy, w, h, true);
	hostScreen.update(dx, dy, w, h, true);

	return 1;
}

/**
 * Draw a coloured line between two points
 *
 * c_draw_line(Virtual *vwk, long x1, long y1, long x2, long y2,
 *                          long pattern, long colour)
 * draw_line
 * In:  a1  VDI struct
 *  d0  logic operation
 *  d1  x1 or table address
 *  d2  y1 or table length (high) and type (low)
 *  d3  x2 or move point count
 *  d4  y2 or move index address
 *  d5  pattern
 *  d6  colour
 *
 * This function has three modes:
 * - single line
 * - table based coordinate pairs (special mode 0 (low word of 'y1'))
 * - table based coordinate pairs+moves (special mode 1)
 *
 * As usual, only the first one is necessary, and a return with d0 = -1
 * signifies that a special mode should be broken down to the basic one.
 *
 * An immediate return with 0 gives a fallback (normally pixel by pixel
 * drawing by the fVDI engine).
 * A negative return will break down the special modes into separate calls,
 * with no more fallback possible.
 **/


/* --------- Clipping routines for box/line */

/* Clipping based heavily on code from                       */
/* http://www.ncsa.uiuc.edu/Vis/Graphics/src/clipCohSuth.c   */

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))


static inline int clipEncode (int x, int y, int left, int top, int right, int bottom)
{
	int code = 0;
	if (x < left) {
		code |= CLIP_LEFT_EDGE;
	} else if (x > right) {
		code |= CLIP_RIGHT_EDGE;
	}
	if (y < top) {
		code |= CLIP_TOP_EDGE;
	} else if (y > bottom) {
		code |= CLIP_BOTTOM_EDGE;
	}
	return code;
}


static inline bool clipLine(int& x1, int& y1, int& x2, int& y2, int cliprect[])
{
#if OLD_CODE // check what is bad!
	if (!cliprect) {
		return true; // Clipping is off
	// Get clipping boundary
 	int left   = cliprect[0];
 	int top    = cliprect[1];
 	int right  = cliprect[2];
 	int bottom = cliprect[3];
#else
	// Get clipping boundary
	int left, top, right, bottom;

	if (!cliprect) {
		left   = 0;
		top    = 0;
		right  = hostScreen.getWidth() -1;
		bottom = hostScreen.getHeight()-1;
	} else {
		left   = cliprect[0];
		top    = cliprect[1];
		right  = cliprect[2];
		bottom = cliprect[3];
	}
#endif

	bool draw = false;
	while (1) {
		int code1 = clipEncode(x1, y1, left, top, right, bottom);
		int code2 = clipEncode(x2, y2, left, top, right, bottom);
		if (CLIP_ACCEPT(code1, code2)) {
			draw = true;
			break;
		} else if (CLIP_REJECT(code1, code2)) {
			break;
		} else {
			if (CLIP_INSIDE(code1)) {
				int swaptmp = x2; x2 = x1; x1 = swaptmp;
				swaptmp = y2; y2 = y1; y1 = swaptmp;
				swaptmp = code2; code2 = code1; code1 = swaptmp;
			}
			float m = 1.0f;
			if (x2 != x1) {
				m = (y2 - y1) / (float)(x2 - x1);
			}
			if (code1 & CLIP_LEFT_EDGE) {
				y1 += (int)((left - x1) * m);
				x1 = left;
			} else if (code1 & CLIP_RIGHT_EDGE) {
				y1 += (int)((right - x1) * m);
				x1 = right;
			} else if (code1 & CLIP_BOTTOM_EDGE) {
				if (x2 != x1) {
					x1 += (int)((bottom - y1) / m);
				}
				y1 = bottom;
			} else if (code1 & CLIP_TOP_EDGE) {
				if (x2 != x1) {
					x1 += (int)((top - y1) / m);
				}
				y1 = top;
			}
		}
	}

	D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineEND", x1, y1, x2, y2));

	return draw;
}


// Don't forget rotation of pattern!
int SoftVdiDriver::drawSingleLine(int x1, int y1, int x2, int y2, uint16 pattern,
                               uint32 fgColor, uint32 bgColor, int logOp, bool last_pixel,
                               int cliprect[], int minmax[])
{
	if (clipLine(x1, y1, x2, y2, cliprect)) {	// Do not draw the line when it is out
		D(bug("fVDI: %s %d,%d:%d,%d (%lx,%lx)", "drawSingleLine", x1, y1, x2, y2, fgColor, bgColor));
		hostScreen.drawLine(x1, y1, x2, y2, pattern, fgColor, bgColor, logOp, last_pixel);
		if (x1 < x2) {
			if (x1 < minmax[0])
				minmax[0] = x1;
			if (x2 > minmax[2])
				minmax[2] = x2;
		} else {
			if (x2 < minmax[0])
				minmax[0] = x2;
			if (x1 > minmax[2])
				minmax[2] = x1;
		}
		if (y1 < y2) {
			if (y1 < minmax[1])
				minmax[1] = y1;
			if (y2 > minmax[3])
				minmax[3] = y2;
		} else {
			if (y2 < minmax[1])
				minmax[1] = y2;
			if (y1 > minmax[3])
				minmax[3] = y1;
		}
	}
	return 1;
}


// Don't forget rotation of pattern!
int SoftVdiDriver::drawTableLine(memptr table, int length, uint16 pattern,
                              uint32 fgColor, uint32 bgColor, int logOp,
                              int cliprect[], int minmax[])
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	for(--length; length > 0; length--) {
		int x2 = (int16)ReadInt16(table); table+=2;
		int y2 = (int16)ReadInt16(table); table+=2;

		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, length == 1, cliprect, minmax);
		x1 = x2;
		y1 = y2;
	}

	return 1;
}


// Don't forget rotation of pattern!
int SoftVdiDriver::drawMoveLine(memptr table, int length, memptr index, int moves, uint16 pattern,
                             uint32 fgColor, uint32 bgColor, int logOp,
                             int cliprect[], int minmax[])
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	moves *= 2;
	moves-=2;
	if ((int16)ReadInt16(index + moves) == -4)
		moves-=2;
	if ((int16)ReadInt16(index + moves) == -2)
		moves-=2;
	int movepnt = -1;
	if (moves >= 0)
		movepnt = ((int16)ReadInt16(index + moves) + 4) / 2;
	for(int n = 1; n < length; n++) {
		int x2 = (int16)ReadInt16(table); table+=2;
		int y2 = (int16)ReadInt16(table); table+=2;
		if (n == movepnt) {
			moves-=2;
			if (moves >= 0)
				movepnt = ((int16)ReadInt16(index + moves) + 4) / 2;
			else
				movepnt = -1;		/* Never again equal to n */
			x1 = x2;
			y1 = y2;
			continue;
		}

		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, length == 1, cliprect, minmax);

		x1 = x2;
		y1 = y2;
	}

	return 1;
}

int32 SoftVdiDriver::drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
	uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
	uint32 logOp, memptr clip)
{
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	memptr table = 0;
	memptr index = 0;
	int length = 0;
	int moves = 0;

	int16 x1 = x1_;
	int16 y1 = y1_;
	int16 x2 = x2_;
	int16 y2 = y2_;

	if (vwk & 1) {
		if ((unsigned)(y1 & 0xffff) > 1)
			return -1;		/* Don't know about this kind of table operation */
		table = (memptr)x1_;
		length = (y1_ >> 16) & 0xffff;
		if ((y1_ & 0xffff) == 1) {
			index = (memptr)y2_;
			moves = x2_ & 0xffff;
		}
		vwk -= 1;
		x1 = (int16)ReadInt16(table);
		y1 = (int16)ReadInt16(table + 2);
		x2 = (int16)ReadInt16(table + 4);
		y2 = (int16)ReadInt16(table + 6);
	}

	int cliparray[4];
	int* cliprect = 0;
	if (clip) {				// Clipping is not off
		cliprect = cliparray;
		cliprect[0] = (int16)ReadInt32(clip);
		cliprect[1] = (int16)ReadInt32(clip + 4);
		cliprect[2] = (int16)ReadInt32(clip + 8);
		cliprect[3] = (int16)ReadInt32(clip + 12);
		D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineTO", cliprect[0], cliprect[1],
		       cliprect[2], cliprect[3]));
	}

	int minmax[4] = {1000000, 1000000, -1000000, -1000000};

#if TEST_STRAIGHT	// Not yet working
	int eq_coord = (x1 == x2) + 2 * (y1 == y2);
#endif

	if (table) {
		if (moves)
			drawMoveLine(table, length, index, moves, pattern, fgColor, bgColor,
			             logOp, cliprect, minmax);
		else {
#if TEST_STRAIGHT	// Not yet working
			if (eq_coord && ((pattern & 0xffff) == 0xffff) && (logOp < 3)) {
				table += 8;
				for(--length; length > 0; length--) {
					if (eq_coord & 1) {
						if (y1 < y2)
							vertical(x1, y1, 1, y2 - y1 + 1);
						else
							vertical(x2, y2, 1, y1 - y2 + 1);
					} else if (eq_coord & 2) {
						if (x1 < x2)
							horizontal(x1, y1, x2 - x1 + 1, 1);
						else
							horizontal(x2, y2, x1 - x2 + 1, 1);
					} else {
						length++;
						table -= 8;
						break;
					}
					x1 = x2;
					y1 = y2;
					x2 = (int16)ReadInt16(table); table+=2;
					y2 = (int16)ReadInt16(table); table+=2;
					eq_coord = (x1 == x2) + 2 * (y1 == y2);
				}
			}
#endif
			switch (length) {
			case 0:
				break;
			case 1:
				drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
				               logOp, true, cliprect, minmax);
				break;
			default:
				drawTableLine(table, length, pattern, fgColor, bgColor,
				              logOp, cliprect, minmax);
 				break;
			}
 		}
#if TEST_STRAIGHT	// Not yet working
	} else if (eq_coord && ((pattern & 0xffff) == 0xffff)) {
		if (eq_coord & 1) {
			if (y1 < y2)
				vertical(x1, y1, 1, y2 - y1 + 1);
			else
				vertical(x2, y2, 1, y1 - y2 + 1);
		} else {
			if (x1 < x2)
				horizontal(x1, y1, x2 - x1 + 1, 1);
			else
				horizontal(x2, y2, x1 - x2 + 1, 1);
		}
#endif
	} else
		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, true, cliprect, minmax);

	if (minmax[0] != 1000000) {
		D(bug("fVDI: %s %d,%d:%d,%d", "drawLineUp",
		       minmax[0], minmax[1], minmax[2], minmax[3]));
		hostScreen.update(minmax[0], minmax[1],
		                  minmax[2] - minmax[0] + 1, minmax[3] - minmax[1] + 1, true);
	} else {
		D2(bug("fVDI: drawLineUp nothing to redraw"));
	}

	return 1;
}

int32 SoftVdiDriver::fillPoly(memptr vwk, memptr points_addr, int n,
	memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
	uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip)
{
#if 0
	return VdiDriver::fillPoly(vwk, points_addr, n, index_addr, moves,
	                           pattern_addr, fgColor, bgColor, logOp,
	                           interior_style, clip);
#else
	DUNUSED(interior_style);

	if (vwk & 1)
		return -1;      // Don't know about any special fills

	// Allocate arrays for data
	if (!AllocPoints(n) || !AllocIndices(moves) || !AllocCrossings(200))
		return -1;

	uint16 pattern[16];
	for(int i = 0; i < 16; ++i)
		pattern[i] = ReadInt16(pattern_addr + i * 2);

	int cliparray[4];
	int* cliprect = 0;
	if (clip) {     // Clipping is not off
		cliprect = cliparray;
		cliprect[0] = (int16)ReadInt32(clip);
		cliprect[1] = (int16)ReadInt32(clip + 4);
		cliprect[2] = (int16)ReadInt32(clip + 8);
		cliprect[3] = (int16)ReadInt32(clip + 12);
		D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineTO", cliprect[0], cliprect[1],
		       cliprect[2], cliprect[3]));
	}

	Points p(alloc_point);
	int16* index = alloc_index;
	int16* crossing = alloc_crossing;

	for(int i = 0; i < n; ++i) {
		p[i][0] = (int16)ReadInt16(points_addr + i * 4);
		p[i][1] = (int16)ReadInt16(points_addr + i * 4 + 2);
	}
	bool indices = moves;
	for(int i = 0; i < moves; ++i)
		index[i] = (int16)ReadInt16(index_addr + i * 2);


	if (!n)
		return 1;

	if (!hostScreen.renderBegin())
		return 1;

	if (!indices) {
		if ((p[0][0] == p[n - 1][0]) && (p[0][1] == p[n - 1][1]))
			n--;
	} else {
		moves--;
		if (index[moves] == -4)
			moves--;
		if (index[moves] == -2)
			moves--;
	}

	int miny = p[0][1];
	int maxy = miny;
	for(int i = 1; i < n; ++i) {
		int16 y = p[i][1];
		if (y < miny) {
			miny = y;
		}
		if (y > maxy) {
			maxy = y;
		}
	}
	if (cliprect) {
		if (miny < cliprect[1])
			miny = cliprect[1];
		if (maxy > cliprect[3])
			maxy = cliprect[3];
	}

	int minx = 1000000;
	int maxx = -1000000;

	for(int16 y = miny; y <= maxy; ++y) {
		int ints = 0;
		int16 x1 = 0;   // Make the compiler happy with some initializations
		int16 y1 = 0;
		int16 x2 = 0;
		int16 y2 = 0;
		int move_n = 0;
		int movepnt = 0;
		if (indices) {
			move_n = moves;
			movepnt = (index[move_n] + 4) / 2;
			x2 = p[0][0];
			y2 = p[0][1];
		} else {
			x1 = p[n - 1][0];
			y1 = p[n - 1][1];
		}

		for(int i = indices; i < n; ++i) {
			if (EnoughCrossings(ints + 1) || AllocCrossings(ints + 1))
				crossing = alloc_crossing;
			else
				break;          // At least something will get drawn

			if (indices) {
				x1 = x2;
				y1 = y2;
			}
			x2 = p[i][0];
			y2 = p[i][1];
			if (indices) {
				if (i == movepnt) {
					if (--move_n >= 0)
						movepnt = (index[move_n] + 4) / 2;
					else
						movepnt = -1;           // Never again equal to n
					continue;
				}
			}

			if (y1 < y2) {
				if ((y >= y1) && (y < y2)) {
					crossing[ints++] = SMUL_DIV((y - y1), (x2 - x1), (y2 - y1)) + x1;
				}
			} else if (y1 > y2) {
				if ((y >= y2) && (y < y1)) {
					crossing[ints++] = SMUL_DIV((y - y2), (x1 - x2), (y1 - y2)) + x2;
				}
			}
			if (!indices) {
				x1 = x2;
				y1 = y2;
			}
		}

		for(int i = 0; i < ints - 1; ++i) {
			for(int j = i + 1; j < ints; ++j) {
				if (crossing[i] > crossing[j]) {
					int16 tmp = crossing[i];
					crossing[i] = crossing[j];
					crossing[j] = tmp;
				}
			}
		}

		x1 = cliprect[0];
		x2 = cliprect[2];
		for(int i = 0; i < ints - 1; i += 2) {
			y1 = crossing[i];       // Really x-values, but...
			y2 = crossing[i + 1];
			if (y1 < x1)
				y1 = x1;
			if (y2 > x2)
				y2 = x2;
			if (y1 <= y2) {
				hostScreen.fillArea(y1, y, y2 - y1 + 1, 1, pattern,
				                    fgColor, bgColor, logOp);
				if (y1 < minx)
					minx = y1;
				if (y2 > maxx)
					maxx = y2;
			}
		}
	}

	hostScreen.renderEnd();
	if (minx != 1000000)
		hostScreen.update(minx, miny, maxx - minx + 1, maxy - miny + 1, true);

	return 1;
#endif
}

int32 SoftVdiDriver::drawText(memptr vwk, memptr text, uint32 length,
			      int32 dst_x, int32 dst_y, memptr font,
			      uint32 w, uint32 h, uint32 fgColor, uint32 bgColor,
			      uint32 logOp, memptr clip)
{
	DUNUSED(vwk);
	DUNUSED(text);
	DUNUSED(length);
	DUNUSED(dst_x);
	DUNUSED(dst_y);
	DUNUSED(font);
	DUNUSED(w);
	DUNUSED(h);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(logOp);
	DUNUSED(clip);

	return 0;
}

void SoftVdiDriver::getHwColor(uint16 index, uint32 red, uint32 green,
	uint32 blue, memptr hw_value)
{
	if (hostScreen.getBpp() != 2)
		// <16bit, 24 and 32bit graphics
		WriteInt32( hw_value,
			    hostScreen.getColor((red * ((1L << 8) - 1) + 500L) / 1000,
		                                (green * ((1L << 8) - 1) + 500L) / 1000,
		                                (blue * ((1L << 8) - 1) + 500L) / 1000) );
	else
		// 5+6+5 rounding needed
		WriteInt32( hw_value,
			    hostScreen.getColor(((red * ((1L << 5) - 1) + 500L) / 1000) << 3,
		                       		((green * ((1L << 6) - 1) + 500L) / 1000) << 2,
		                           	((blue * ((1L << 5) - 1) + 500L) / 1000) << 3) );

	if (hostScreen.getBpp() <= 1) {
		WriteInt32( hw_value, index );
	}

	D(bug("fVDI: getHwColor (%03d) %04d,%04d,%04d - %lx", index, red, green, blue, ReadInt32( hw_value )));
}

/**
 * Set palette colour (hooked into the c_set_colors driver function by STanda)
 *
 * set_color_hook
 *  4(a7)   paletteIndex
 *  8(a7)   red component byte value
 *  10(a7)  green component byte value
 *  12(a7)  blue component byte value
 **/
 
void SoftVdiDriver::setColor(memptr vwk, uint32 paletteIndex, uint32 red,
	uint32 green, uint32 blue)
{
	DUNUSED(vwk);
	D(bug("fVDI: setColor [NULL]: %03d (%03d) - %04d,%04d,%04d - %02x,%02x,%02x", paletteIndex, toTosColors(paletteIndex) & 0xff, red, green, blue, (uint8)((red * ((1L << 8) - 1) + 500L) / 1000), (uint8)((green * ((1L << 8) - 1) + 500L) / 1000), (uint8)((blue * ((1L << 8) - 1) + 500L) / 1000) ));

	if (hostScreen.getBpp() != 2)
		// <16bit, 24 and 32bit graphics
		hostScreen.setPaletteColor(toTosColors(paletteIndex),
		                           (red * ((1L << 8) - 1) + 500L) / 1000,
		                           (green * ((1L << 8) - 1) + 500L) / 1000,
		                           (blue * ((1L << 8) - 1) + 500L) / 1000);
	else
		// 5+6+5 rounding needed
		hostScreen.setPaletteColor(toTosColors(paletteIndex),
		                           ((red * ((1L << 5) - 1) + 500L) / 1000) << 3,
		                           ((green * ((1L << 6) - 1) + 500L) / 1000) << 2,
		                           ((blue * ((1L << 5) - 1) + 500L) / 1000) << 3);

	hostScreen.updatePalette( 256 );
}

int32 SoftVdiDriver::getFbAddr(void)
{
	int32 result;

#ifdef FIXED_VIDEORAM
	result = ARANYMVRAMSTART;
#else
	result = VideoRAMBase;
#endif
	D(bug("fVDI: getVideoramAddress: %#lx", result));
	return result;
}

