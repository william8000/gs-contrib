/* Copyright (C) 1991 Free Software Foundation, Inc.  All rights reserved.

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

/*
gdevci.c - GhostScript driver for CIE CI-4000 Tri Printer Model 20
26Sep91 - wb - created from gdevln03.c by Ulrich Mueller <ulm@vsnhd1.cern.ch>

Makfile lines:
###### ----------------------- The ci4000 device -------------------- ######

ci_=gdevci.$(OBJ)
ci.dev: $(ci_)
	.$(DS)gssetdev ci $(ci_)

cilq_=gdevci.$(OBJ)
cilq.dev: $(cilq_)
	.$(DS)gssetdev cilq $(cilq_)

gdevci.$(OBJ): gdevci.c $(GDEV)
*/

#include "gdevprn.h"

#ifdef __GCC__
#define	HAS_ALLOCA
#endif

/* The device descriptors */
private dev_proc_print_page(cilq_print_page);
private dev_proc_print_page(ci_print_page);

gx_device_printer gs_cilq_device =
    prn_device(prn_std_procs, "cilq",
		131,			/* width_10ths,  3144 pixels */
		110,			/* height_10ths, 1584 pixels */
		240, 144,		/* x_dpi, y_dpi */
		0, 0, 0, 0,		/* margins */
		1, cilq_print_page);

gx_device_printer gs_cilqb_device =
    prn_device(prn_std_procs, "cilqb",
		131,			/* width_10ths,  3144 pixels */
		350,			/* height_10ths */
		240, 144,		/* x_dpi, y_dpi */
		0, 0, 0, 0,		/* margins */
		1, cilq_print_page);

gx_device_printer gs_ci_device =
    prn_device(prn_std_procs, "ci",
		131,			/* width_10ths,  3144 pixels */
		110,			/* height_10ths, 792 pixels */
		240, 72,		/* x_dpi, y_dpi */
		0, 0, 0, 0,		/* margins */
		1, ci_print_page);

gx_device_printer gs_cib_device =
    prn_device(prn_std_procs, "cib",
		131,			/* width_10ths,  3144 pixels */
		350,			/* height_10ths */
		240, 72,		/* x_dpi, y_dpi */
		0, 0, 0, 0,		/* margins */
		1, ci_print_page);

/* ------ Internal routines ------ */

/* Send the page to the printer. */
private int
cilq_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
    byte *in, *inp;
    int lnum, lcount, l, count, empty, mask, line_size;
    int ctop, cbot, oldctop, oldcbot;

    line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
#ifdef HAS_ALLOCA
    in = (byte *)alloca(line_size * 12);
#else
    in = (byte *)gs_malloc(line_size * 12, 1, "in");
#endif

    /* switch to graphics mode, Mode 2, xdpi = 240, ydpi = 144 */
    fputs("\033P12p", prn_stream);

    /* Print lines of graphics */
    for (lnum = lcount = 0; lnum < pdev->height; lnum += 12, lcount++) {
	gdev_prn_copy_scan_lines(pdev, lnum, inp = in, line_size * 12);

	mask = 0200;
	oldctop = oldcbot = 077;
	count = 0;
	empty = 1;

	l = pdev->width;

	while (l-- > 0) {
	    /* transpose two 6*8 rectangles */

	    ctop = cbot = 0x3F;

	    if (inp[0] & mask)
		ctop += 1;
	    if (inp[1*line_size] & mask)
		ctop += 2;
	    if (inp[2*line_size] & mask)
		ctop += 4;
	    if (inp[3*line_size] & mask)
		ctop += 010;
	    if (inp[4*line_size] & mask)
		ctop += 020;
	    if (inp[5*line_size] & mask)
		ctop += 040;

	    if (inp[6*line_size] & mask)
		cbot += 1;
	    if (inp[7*line_size] & mask)
		cbot += 2;
	    if (inp[8*line_size] & mask)
		cbot += 4;
	    if (inp[9*line_size] & mask)
		cbot += 010;
	    if (inp[10*line_size] & mask)
		cbot += 020;
	    if (inp[11*line_size] & mask)
		cbot += 040;

	    if (!(mask >>= 1)) {
		mask = 0200;
		inp++;
	    }

	    if (ctop != oldctop || cbot != oldcbot) {
		if (empty) {
		    while (--lcount >= 0)
			/* terminate previous line */
			fputc('-', prn_stream);
		    empty = lcount = 0;
		}
		while (count >= 99) {
		    fprintf(prn_stream, "!99%c%c", oldctop, oldcbot);
		    count -= 99;
		}
		if (count >= 3)
		    /* use run length encoding */
		    fprintf(prn_stream, "!%d%c%c", count, oldctop, oldcbot);
		else
		    while (count-- > 0) {
			fputc(oldctop,prn_stream);
			fputc(oldcbot,prn_stream);
		    }
		oldctop = ctop;
		oldcbot = cbot;
		count = 0;
	    }
	    count++;
	}

	if (oldctop != 0x3F || oldcbot != 0x3F) {
	    while (count >= 99) {
	        fprintf(prn_stream, "!99%c%c", oldctop, oldcbot);
	        count -= 99;
	    }
	    if (count >= 3)
	        /* use run length encoding */
	        fprintf(prn_stream, "!%d%c%c", count, oldctop, oldcbot);
	    else
	        while (count-- > 0) {
		    fputc(oldctop,prn_stream);
		    fputc(oldcbot,prn_stream);
	        }
	}
    }

    /* terminate page */
    fputs("\030\f", prn_stream); /* CAN to terminate Graphics, Form feed */
    fflush(prn_stream);

#ifndef HAS_ALLOCA
    gs_free(in, line_size * 12, 1, "in");
#endif

    return(0);
}

private int
ci_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
    byte *in, *inp;
    int lnum, lcount, l, count, empty, mask, line_size;
    int c, oldc;

    line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
#ifdef HAS_ALLOCA
    in = (byte *)alloca(line_size * 6);
#else
    in = (byte *)gs_malloc(line_size * 6, 1, "in");
#endif

    /* switch to graphics mode, Mode 2, xdpi = 240, ydpi = 72 */
    fputs("\033P3q", prn_stream);

    /* Print lines of graphics */
    for (lnum = lcount = 0; lnum < pdev->height; lnum += 6, lcount++) {
	gdev_prn_copy_scan_lines(pdev, lnum, inp = in, line_size * 6);

	mask = 0200;
	c = 077;
	oldc = 0;
	count = 0;
	empty = 1;

	l = pdev->width;

	while (l-- > 0) {
	    /* transpose a 6*8 rectangles */

	    c = 0x3F;

	    if (inp[0] & mask)
		c += 1;
	    if (inp[1*line_size] & mask)
		c += 2;
	    if (inp[2*line_size] & mask)
		c += 4;
	    if (inp[3*line_size] & mask)
		c += 010;
	    if (inp[4*line_size] & mask)
		c += 020;
	    if (inp[5*line_size] & mask)
		c += 040;

	    if (!(mask >>= 1)) {
		mask = 0200;
		inp++;
	    }

	    if (c != oldc) {
		if (empty) {
		    while (--lcount >= 0)
			/* terminate previous line */
			fputc('-', prn_stream);
		    empty = lcount = 0;
		}
		while (count >= 99) {
		    fprintf(prn_stream, "!99%c", oldc);
		    count -= 99;
		}
		if (count >= 3)
		    /* use run length encoding */
		    fprintf(prn_stream, "!%d%c", count, oldc);
		else
		    while (count-- > 0)
			fputc(oldc,prn_stream);
		oldc = c;
		count = 0;
	    }
	    count++;
	}

	if (oldc != 0x3F) {
	    while (count >= 99) {
	        fprintf(prn_stream, "!99%c", oldc);
	        count -= 99;
	    }
	    if (count >= 3)
	        /* use run length encoding */
	        fprintf(prn_stream, "!%d%c", count, oldc);
	    else
	        while (count-- > 0)
		    fputc(oldc,prn_stream);
	}
    }

    /* terminate page */
    fputs("\030\f", prn_stream); /* CAN to terminate Graphics, Form feed */
    fflush(prn_stream);

#ifndef HAS_ALLOCA
    gs_free(in, line_size * 6, 1, "in");
#endif

    return(0);
}
