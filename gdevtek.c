/* Copyright (C) 1989, 1990, 1991 Aladdin Enterprises.  All rights reserved.
   Distributed by Free Software Foundation, Inc.

This file is part of Ghostscript.

Ghostscript is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves any
particular purpose or works at all, unless he says so in writing.  Refer
to the Ghostscript General Public License for full details.

Everyone is granted permission to copy, modify and redistribute
Ghostscript, but only under the conditions described in the Ghostscript
General Public License.  A copy of this license is supposed to have been
given to you along with Ghostscript so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies.  */

/* gdevtek.c 31Jul91 - wb - created from gdevepsn.c */
/* Tektronics terminal driver for Ghostscript */
/* 1024 X 768 Graph Mode using 10-bit addressing */

/* Use the makefile lines:
tek_=gdevtek.$(OBJ) gdevprn.$(OBJ)
tek.dev: $(tek_)
	gssetdev tek.dev $(tek_)
gdevtek.$(OBJ): gdevtek.c $(GDEV)
 *
 * and then add tek entries to DEVICES, DEVICE_DEVS and DEVICE_OBJS
 */

#include "gdevprn.h"

#if defined(__STDC__)
#include "stdlib.h"
#else
extern char *getenv(P1(const char *));
#endif

/* The device descriptor */
static dev_proc_print_page(tek_print_page);
gx_device_printer gs_tek_device =
  prn_device(prn_std_procs, "tek",
	89,				/* width_10ths */
	60,				/* height_10ths */
	115,				/* xdpi */
	128,				/* ydpi */
	0, 0, 0, 0,			/* margins */
	1, tek_print_page);

/* ------ Internal routines ------ */

/* Last X and Y locations */

static int tek_x_high, tek_y_high, tek_y_low;

/* Goto a given position, using a short address when possible */
static void
tek_gotoxy(int x, int y, gp_file *prn_stream)
{
	int xh = (x >> 5);
	int xl = (x & 0x1F);
	int yh = (y >> 5);
	int yl = (y & 0x1F);

	if (yh != tek_y_high) {
		gp_fputc(0x20 | yh, prn_stream);	/* Y-high */
		tek_y_high = yh;
	}
	if (yl != tek_y_low || xh != tek_x_high) {
		gp_fputc(0x60 | yl, prn_stream);	/* Y-low */
		tek_y_low = yl;
	}
	if (xh != tek_x_high) {
		gp_fputc(0x20 | xh, prn_stream);	/* X-high */
		tek_x_high = xh;
	}
	gp_fputc(0x40 | xl, prn_stream);		/* X-low */
}

/* Send the page to the printer. */
static int
tek_print_page(gx_device_printer *pdev, gp_file *prn_stream)
{
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	byte *in = (byte *)gs_malloc(pdev->memory, line_size+1, 1, "tek_print_page(in)");
	int lnum;
	int to_stdout;
	char *s;

	/* Check allocation */
	if ( in == 0 )
	   {    gs_free(pdev->memory, in, line_size+1, 1, "tek_print_page(in)");
		return -1;
	   }

	/* Select output device, set GSDUMP=1 to dump to a file */
	to_stdout = 0;
	s = getenv("GSDUMP");
	if (s == NULL || *s != '1') {
		to_stdout = 1;
#if 0
		prn_stream = stdout;
#endif
	}

	/* Clear the screen */
	if (to_stdout)
		gp_fputs("\033[H\033[J", prn_stream);

	/* Select Solid lines, normal intensity */
	gp_fputs("\033e", prn_stream);

	/* Clear saved coordinates */
	tek_x_high = tek_y_high = tek_y_low = -1;

	/* Print lines */
	for ( lnum = 0; lnum < pdev->height; lnum++ )
	   {
		int pos = 0;
		int bit = 7;
		int y = pdev->height - lnum - 1;

		/* Copy a scan line */
		gdev_prn_copy_scan_lines(pdev, lnum, in, line_size);

		while ( pos < line_size ) {
		     	/* skip 0 bits */
			if (bit < 7) {
				while (bit >= 0 && (in[pos]&(1<<bit)) == 0)
					bit--;
				if (bit < 0) {
					pos++;
					if (pos >= line_size) break;
					bit = 7;
				}
			}

			if (bit == 7) {
				/* skip 0 bytes */
				in[line_size] = 0xFF;
				while (in[pos] == 0) pos++;

				if (pos >= line_size) break;

				/* skip 0 bits */
				while ((in[pos]&(1<<bit)) == 0)
					--bit;
			}


			/* start a vector */
			gp_fputc(29, prn_stream);	/* GS = graphic start */
			tek_gotoxy(pos*8 + 7 - bit, y, prn_stream);

			/* skip 1 bits */
			while (bit >= 0 && (in[pos]&(1<<bit)) != 0)
				--bit;

			if (bit < 0) {
				/* skip all set bytes */
				in[line_size] = 0;
				pos++;
				while (in[pos] == 0xFF)
					pos++;
				bit = 7;
				if (pos < line_size) {
					/* skip 1 bits */
					while ((in[pos]&(1<<bit)) != 0)
						--bit;
				}
			}

			/* finish the vector */
			tek_gotoxy(pos*8 + 7 - bit, y, prn_stream);
		}
	   }

	/* Enter alpha mode and home cursor */
	if (to_stdout)
		gp_fputs("\n\033[H", prn_stream);

	gs_free(pdev->memory, in, line_size+1, 1, "tek_print_page(in)");
	return 0;
}
