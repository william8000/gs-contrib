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

/* gdevansi.c 30Jul91 - wb - created from gdevepsn.c */
/* Ansi terminal driver for Ghostscript */

/* Use the makefile lines:
ansi_=gdevansi.$(OBJ) gdevprn.$(OBJ)
ansi.dev: $(ansi_)
	.$(DS)gssetdev ansi $(ansi_)
gdevansi.$(OBJ): gdevansi.c $(GDEV)
 *
 * and then add ansi entries to DEVICES and DEVICE_DEVS
 */

#include "gdevprn.h"

#if defined(__STDC__)
#include "stdlib.h"
#else
extern char *getenv(P1(const char *));
#endif

/* The device descriptor */
private dev_proc_print_page(ansi_print_page);
gx_device_printer gs_ansi_device =
  prn_device(prn_std_procs, "ansi",
	80,				/* width_10ths */
	60,				/* height_10ths */
	10, 4,				/* xdpi, ydpi */
	0, 0, 0, 0,			/* margins */
	1, ansi_print_page);

/* ------ Internal routines ------ */

/* Send the page to the printer. */
private int
ansi_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	int in_size = line_size * 8;
	byte *in = (byte *)gs_malloc(in_size, 1, "ansi_print_page(in)");
	int lnum;
	char *s;

	/* Check allocations */
	if ( in == 0 )
	   {	if ( in ) gs_free(in, in_size, 1, "ansi_print_page(in)");
		return -1;
	   }

	/* Select output device, set GSDUMP=1 to dump to a file */
	s = getenv("GSDUMP");
	if (s == NULL || *s != '1')
		prn_stream = stdout;

	/* Clear the screen */
	fputs("\033[H\033[J", prn_stream);

	/* Print lines of graphics */
	lnum = 0;
	while ( lnum < pdev->height )
	   {	int pos, bit, skip, len;
		char num[10];

		/* Copy 1 scan line */
		gdev_prn_copy_scan_lines(pdev, lnum, in, line_size);

		skip = 0;
		for (pos = 0; pos < line_size; pos++) {
			for(bit = 7; bit >= 0; --bit) {
				if ((in[pos]&(1<<bit)) != 0) {
					if (skip > 5)  {
						fputc('\033', prn_stream);
						fputc('[', prn_stream);
						len = 0;
						do {
							num[len++] = skip % 10;
							skip /= 10;
						} while (skip > 0);
						while (len-- > 0)
							fputc(num[len]+'0',prn_stream);
						fputc('C', prn_stream);
					} else if (skip > 0) {
						while (skip-- > 0)
							fputc(' ', prn_stream);
					}
					fputc('*', prn_stream);
				} else {
					++skip;
				}
			}
		}
		lnum++;
		if (lnum < pdev->height)
			fputc('\n', prn_stream);
	   }

	/* Put cursor at top of screen */
	fputs("\033[H", prn_stream);

	fflush(prn_stream);

	gs_free(in, in_size, 1, "ansi_print_page(in)");
	return 0;
}
