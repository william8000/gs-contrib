/* Copyright (C) 1989, 1990, 1991 Aladdin Enterprises.	All rights reserved.
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

/* gdevsixel.c 30Jul91 - wb - created from gdevepsn.c */
/*		3Sep91 - wb - added repeat thresold set by GSREPEAT */
/*	       27Sep91 - wb - made inner loop like gdevln03.c */
/*	       22Mar96 - wb - added invert flags set by GSINVERT */
/* sixel terminal driver for Ghostscript */
/* Each sixel is a slice of 6 bits */
/* The screen is 1120 (=80X14) sixels (1120 bits) wide and */
/* 67 sixels (67X6 = 402 bits) high. */

/* Use the makefile lines:
sixel_=gdevsixel.$(OBJ) gdevprn.$(OBJ)
sixel.dev: $(sixel_)
	.$(DS)gssetdev sixel $(sixel_)
sixelk_=gdevsixel.$(OBJ) gdevprn.$(OBJ)
sixelk.dev: $(sixelk_)
	.$(DS)gssetdev sixelk $(sixelk_)
gdevsixel.$(OBJ): gdevsixel.c $(GDEV)
 *
 * and then add sixel entries to DEVICES and DEVICE_DEVS
 */

#include "gdevprn.h"

#if defined(__STDC__)
#include "stdlib.h"
#else
extern char *getenv(P1(const char *));
extern int atoi(P1(const char *));
#endif

/* The device descriptor */
static dev_proc_print_page(sixel_print_page);

/* Sixel parameters for a vt300 */
const gx_device_printer far_data gs_sixel_device =
  prn_device(gdev_prn_initialize_device_procs_mono_bg, "sixel",
	90,				/* width_10ths */
	60,				/* height_10ths */
	124,				/* xdpi */
	67,				/* ydpi */
	0, 0, 0, 0,			/* margins */
	1, sixel_print_page);

/* Sixel parameters for kermit */
const gx_device_printer far_data gs_sixelk_device =
  prn_device(gdev_prn_initialize_device_procs_mono_bg, "sixelk",
	90,				/* width_10ths */
	60,				/* height_10ths */
	71,				/* xdpi */
	58,				/* ydpi */
	0, 0, 0, 0,			/* margins */
	1, sixel_print_page);

/* ------ Internal routines ------ */

/* Send the page to the printer. */
static int
sixel_print_page(gx_device_printer *pdev, gp_file *prn_stream)
{
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	int in_size = line_size * 6;
	byte *in = (byte *)gs_malloc(pdev->memory, in_size, 1, "sixel_print_page(in)");
	int lnum;
	int repeat_limit;
	int invert;
	int to_stdout;
	char *s;

	/* Check allocation */
	if ( in == 0 )
	   {	gs_free(pdev->memory, in, in_size, 1, "sixel_print_page(in)");
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

	/* Select repeat theshold, default = 3 */
	s = getenv("GSREPEAT");
	repeat_limit = ((s == NULL || *s == '\0')? 3: atoi(s));

	/* Select invert flag */
	s = getenv("GSINVERT");
	invert = ((s == NULL || *s == '\0')? 1: atoi(s));

	/* Clear the screen */
	if (to_stdout)
		gp_fputs("\033[H\033[J", prn_stream);

	/* Start sixel sequence ESC P 7 (1:1 aspect ratio) q */
	gp_fputs("\033P7q", prn_stream);

	/* Print lines with 6 pixel height of graphics */
	for ( lnum = 0; lnum+5 < pdev->height; lnum += 6 )
	   {
		byte *inp = in;
		int sixel = 0;
		int mask = 0200;
		int i;
		int last_sixel = 0;
		int repeat_count = 0;

		/* Copy 6 scan lines */
		gdev_prn_copy_scan_lines(pdev, lnum, in, line_size*6);

		i = pdev->width;
		while ( i-- > 0 ) {
		    sixel = 0;
		    if (invert) {
			/* black text on white background */
			if (!(inp[0]		& mask)) sixel |= 001;
			if (!(inp[line_size]	& mask)) sixel |= 002;
			if (!(inp[2*line_size]	& mask)) sixel |= 004;
			if (!(inp[3*line_size]	& mask)) sixel |= 010;
			if (!(inp[4*line_size]	& mask)) sixel |= 020;
			if (!(inp[5*line_size]	& mask)) sixel |= 040;
		    } else {
			/* white text on black background */
			if (inp[0]		& mask) sixel |= 001;
			if (inp[line_size]	& mask) sixel |= 002;
			if (inp[2*line_size]	& mask) sixel |= 004;
			if (inp[3*line_size]	& mask) sixel |= 010;
			if (inp[4*line_size]	& mask) sixel |= 020;
			if (inp[5*line_size]	& mask) sixel |= 040;
		    }
		    if (!(mask >>= 1)) {
			mask = 0200;
			inp++;
		    }
		    if (sixel != last_sixel) {
			if (repeat_count > repeat_limit) {
			    gp_fprintf(prn_stream,
				"!%d%c", repeat_count, last_sixel+0x3F);
			} else {
			    while (repeat_count-- > 0)
				gp_fputc(last_sixel+0x3F, prn_stream);
			}
			repeat_count = 1;
			last_sixel = sixel;
		    } else {
			++repeat_count;
		    }
		}

		/* dump last sixel if non-blank */
		if (sixel > 0) {
			if (repeat_count > repeat_limit) {
				gp_fprintf(prn_stream,
					"!%d%c", repeat_count, sixel+0x3F);
			} else {
				while (repeat_count-- > 0)
					gp_fputc(sixel+0x3F, prn_stream);
			}
		}

		/* send a graphics new-line to start next line */
		gp_fputc('-', prn_stream);
	   }

	/* Terminate sixel sequence with ESC backslash */
	gp_fputs("\033\134", prn_stream);
	   
	/* Home cursor */
	if (to_stdout)
		gp_fputs("\033[H", prn_stream);

	gs_free(pdev->memory, in, in_size, 1, "sixel_print_page(in)");
	return 0;
}
