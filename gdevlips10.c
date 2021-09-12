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

/* gdevlips.c 27Sep91 wb created from gdevtek.c */
/* C.Itoh LIPS 10 Laser Printer driver for Ghostscript */
/* 300 X 300 dpi on 8 1/2 X 11 paper */
/* 19Feb10 wb renamed to lips10 to avoid conflict with contributed lips device */

/* Use the makefile lines:
 *
lips10_=$(GLOBJ)gdevlips10.$(OBJ)
$(DD)lips10.dev: $(lips10_)
	$(SETDEV) $(DD)lips10 $(lips10_)

$(GLOBJ)gdevlips10.$(OBJ): $(GLSRC)gdevlips10.c $(GDEV)
	$(GLCC) $(GLO_)gdevlips10.$(OBJ) $(C_) $(GLSRC)gdevlips10.c
 *
 * old versions of gs
 *
lips10_=gdevlips10.$(OBJ)
lips10.dev: $(lips10_)
	.$(DS)gssetdev lips10 $(lips10_)
gdevlips10.$(OBJ): gdevlips10.c $(GDEV)
 *
 * and then add lips10 entries to DEVICES and DEVICE_DEVS
 */

#include "gdevprn.h"

/* The device descriptor */
static dev_proc_print_page(lips10_print_page);
gx_device_printer gs_lips10_device =
  prn_device(prn_std_procs, "lips10",
	85,				/* width_10ths */
	110,				/* height_10ths */
	300,				/* xdpi */
	300,				/* ydpi */
	0, 0, 0, 0,			/* margins */
	1, lips10_print_page);

/* ------ Internal routines ------ */

/* Send the page to the printer. */
static int
lips10_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	byte *in = (byte *)gs_malloc(pdev->memory, line_size+1, 1, "lips10_print_page(in)");
	int lnum;

	/* Check allocation */
	if ( in == 0 )
	   {    gs_free(pdev->memory, in, line_size+1, 1, "lips10_print_page(in)");
		return -1;
	   }

	/* Enter graphics mode, set units to dots, landscape, set pen width */
	fputs("(O] UNIT D; SPO L; SPD 1;\n", prn_stream);

	/* Print lines */
	for ( lnum = 0; lnum < pdev->height; lnum++ )
	   {
		int pos = 0;
		int bit = 7;
		int y = lnum;
		int x;
		int lastx = 0;
		int first = 1;

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


			/* start a line */
			x = pos*8 + 7 - bit;
			if (first) {
				fprintf(prn_stream, "MAP %d,%d;", x, y);
				first = 0;
			} else {
				fprintf(prn_stream, "MRP %d,0;", x-lastx);
			}
			lastx = x;

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
			x = pos*8 + 7 - bit;
			fprintf(prn_stream, "DRP %d,0;\n", x-lastx);
			lastx = x;
		}
	   }

	/* Exit graphics mode and flush the page */
	fputs("EXIT; \014", prn_stream);

	gs_free(pdev->memory, in, line_size+1, 1, "lips10_print_page(in)");
	return 0;
}
