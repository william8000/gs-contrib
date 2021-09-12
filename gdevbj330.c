/* Copyright (C) 1990, 1991 Aladdin Enterprises.  All rights reserved.
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

/* gdevbj330.c -- based on gdevbj10.c 17Jul91 wb */
/*		 7Aug91 wb added bj330b, GSFORCE */
/*		12Aug91 wb added check for EOF */
/*		13Aug91 wb added lnummax for bj330b */
/* Canon Bubble Jet BJ-330 printer driver for Ghostscript */

/* makefile lines:
### ----------------- The Canon BubbleJet BJ330 device ----------------- ###

BJ330=gdevbj330.$(OBJ) gdevprn.$(OBJ)

gdevbj330.$(OBJ): gdevbj330.c $(PDEVH) gdevs.mak

# All of these are the same device with different page sizes
# l = 8X11 letter, w = 13X11 wide, d = 13X22, t = 13X33, b = 13X35

bj330l_=$(BJ330)
bj330l.dev: $(bj330l_)
	.$(DS)gssetdev bj330l.dev $(bj330l_)

bj330w_=$(BJ330)
bj330w.dev: $(bj330w_)
	.$(DS)gssetdev bj330w.dev $(bj330w_)

bj330d_=$(BJ330)
bj330d.dev: $(bj330d_)
	.$(DS)gssetdev bj330d.dev $(bj330d_)

bj330t_=$(BJ330)
bj330t.dev: $(bj330t_)
	.$(DS)gssetdev bj330t.dev $(bj330t_)

bj330b_=$(BJ330)
bj330b.dev: $(bj330b_)
	.$(DS)gssetdev bj330b.dev $(bj330b_)
 */
/* Also, add entries to DEVICES, DEVICE_DEVS, and DEVICE_OBJS */

#include "gdevprn.h"

#if defined(__STDC__)
#include "stdlib.h"
#else
extern char *getenv(P1(const char *));
#endif

#ifndef	TRN_COUNT
#  define TRN_COUNT 0			/* extra count calls to transpose */
#endif

/* The device descriptor */
private dev_proc_print_page(bj330_print_page);

	/* 8.5 X 11, letter sized */
gx_device_printer gs_bj330l_device =
  prn_device(prn_std_procs, "bj330l",
	85,				/* width_10ths */
	110,				/* height_10ths " */
	360,				/* x_dpi */
	360,				/* y_dpi */
	0,0,0,0,			/* margins */
	1, bj330_print_page);

	/* 14 X 11, wide sized */
gx_device_printer gs_bj330w_device =
  prn_device(prn_std_procs, "bj330w",
	140,				/* width_10ths */
	110,				/* height_10ths */
	360,				/* x_dpi */
	360,				/* y_dpi */
	0,0,0,0,			/* margins */
	1, bj330_print_page);

	/* 14 x 22, two wide sheets */
gx_device_printer gs_bj330d_device =
  prn_device(prn_std_procs, "bj330d",
	140,				/* width_10ths */
	220,				/* height_10ths */
	360,				/* x_dpi */
	360,				/* y_dpi */
	0,0,0,0,			/* margins */
	1, bj330_print_page);

	/* 14 x 33, three wide sheets */
gx_device_printer gs_bj330t_device =
  prn_device(prn_std_procs, "bj330t",
	140,				/* width_10ths */
	330,				/* height_10ths */
	360,				/* x_dpi */
	360,				/* y_dpi */
	0,0,0,0,			/* margins */
	1, bj330_print_page);

	/* 14 x 35, three wide sheets */
gx_device_printer gs_bj330b_device =
  prn_device(prn_std_procs, "bj330b",
	140,				/* width_10ths */
	350,				/* height_10ths */
	360,				/* x_dpi */
	360,				/* y_dpi */
	0,0,0,0,			/* margins */
	1, bj330_print_page);

/* ------ internal routines ------ */

/* Send the page to the printer. */
private int
bj330_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
	/* 15Jul91 wb changed &pdev->mem to (gx_device *)pdev */
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	byte *in = (byte *)gs_malloc(1, 48*line_size, "bj330_print_page(in)");
	byte *out = (byte *)gs_malloc(1, 48*line_size, "bj330_print_page(out)");
	static char cmp[18] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int lnum, lnummax, skip_count, printed_something;
	int force;
	int status = 0;
	char *s;
#if TRN_COUNT
	long trn_total = 0;	/* total calls */
	long trn_0 = 0;		/* all bytes 0 */
	long trn_1 = 0;		/* all bytes 1 */
	long trn_eq_i = 0;	/* all ints equal */
	long trn_eq_b = 0;	/* all bytes equal */
#endif

	if ( in == 0 || out == 0 )
	   {
		if ( in ) gs_free(in, 1, 48*line_size, "bj330_print_page(in)");
		if ( out ) gs_free(out, 1, 48*line_size, "bj330_print_page(out)");
		return -1;
	   }

	/* Force leading and trailing blank lines if GSFORCE=1 */
	s = getenv("GSFORCE");
	force = (s != NULL && *s == '1');

	/* Linewidth for 48 pixel lines */
	/* ESC A <n> = set line spacing to n / 72 inches */
	/* ESC 2 = invoke variable line spacing (must follow ESC A) */
	fprintf(prn_stream, "\033A%c\033%c", 8, '2');

	/* Turn off auto LF mode */
	/* ESC 5 <n> = set auto LF mode flag to n, 0=off, 1=on */
	/* note that we could save one char per line, but this way */
	/* our output has LF characters so it is easier to view under unix */
	fprintf(prn_stream, "\033%c%c", '5', 0);

	skip_count = 0;
	printed_something = 0;

	lnummax = pdev->height;

	/* Print lines with 48 pixel height of graphics */
	for ( lnum = 0 ; lnum < pdev->height; lnum += 48 )
	   {	byte *inp = in;
		byte *outp = out;
		byte *in_end = in + line_size;
		byte *out_beg = out;
		byte *out_end = out + 6 * pdev->width;
		byte *out_last;		/* 1+last position scanned */
		byte *out_pos;		/* used as temp variable */
		int count;

		gdev_prn_copy_scan_lines(pdev, lnum, inp, line_size*48);

		while ( inp < in_end )
		   {
		   	int i;
			for ( i = 0; i < 6; i++, outp++ )
#if 0
				gdev_prn_transpose_8x8(inp + 8*i*line_size,
							line_size, outp, 6);
#else
{
	/* inline version of prn_transpose_8x8 */

        byte *in_pos = inp + 8*i*line_size;
        byte *out_pos = outp;
        register uint ae = (uint)*in_pos << 8;
	register uint bf = (uint)*(in_pos += line_size) << 8;
	register uint cg = (uint)*(in_pos += line_size) << 8;
	register uint dh = (uint)*(in_pos += line_size) << 8;
	register uint temp;
	ae += *(in_pos += line_size);
	bf += *(in_pos += line_size);
	cg += *(in_pos += line_size);
	dh += in_pos[line_size];

#if TRN_COUNT
	{
		byte *b = inp + 8*i*line_size;
		++trn_total;
		if (*b == b[1*line_size] && *b == b[2*line_size] &&
		    *b == b[3*line_size] && *b == b[4*line_size] &&
		    *b == b[5*line_size] && *b == b[6*line_size] &&
		    *b == b[7*line_size]) {
			if (ae == bf && ae == cg && ae == dh) {
				if (ae == 0) ++trn_0;
				else if (ae == 0xffff) ++trn_1;
				else ++trn_eq_i;
			} else ++trn_eq_b;
		}
	}
#endif

	if (ae == bf && ae == cg && ae == dh) {
		if (ae == 0) {
		out_pos[0] = out_pos[6] = out_pos[12] = out_pos[18] =
		  out_pos[24] = out_pos[30] = out_pos[36] = out_pos[42] = 0;
		} else {
		out_pos[0] = (ae&0x8000?0xf0:0x00) | (ae&0x0080?0x0f:0x00);
		out_pos[6] = (ae&0x4000?0xf0:0x00) | (ae&0x0040?0x0f:0x00);
		out_pos[12]= (ae&0x2000?0xf0:0x00) | (ae&0x0020?0x0f:0x00);
		out_pos[18]= (ae&0x1000?0xf0:0x00) | (ae&0x0010?0x0f:0x00);
		out_pos[24]= (ae&0x0800?0xf0:0x00) | (ae&0x0008?0x0f:0x00);
		out_pos[30]= (ae&0x0400?0xf0:0x00) | (ae&0x0004?0x0f:0x00);
		out_pos[36]= (ae&0x0200?0xf0:0x00) | (ae&0x0002?0x0f:0x00);
		out_pos[42]= (ae&0x0100?0xf0:0x00) | (ae&0x0001?0x0f:0x00);
		}
	} else {

/* Transpose blocks of 4 x 4 */
#define transpose4(r)\
  r ^= (temp = ((r >> 4) ^ r) & 0x00f0);\
  r ^= temp << 4
	transpose4(ae);
	transpose4(bf);
	transpose4(cg);
	transpose4(dh);

/* Transpose blocks of 2 x 2 */
#define transpose(r,s,mask,shift)\
  r ^= (temp = ((s >> shift) ^ r) & mask);\
  s ^= temp << shift
	transpose(ae, cg, 0x3333, 2);
	transpose(bf, dh, 0x3333, 2);

/* Transpose blocks of 1 x 1 */
	transpose(ae, bf, 0x5555, 1);
	transpose(cg, dh, 0x5555, 1);

	*out_pos = ae >> 8;
	out_pos += 6;
	*out_pos = bf >> 8;
	out_pos += 6;
	*out_pos = cg >> 8;
	out_pos += 6;
	*out_pos = dh >> 8;
	out_pos += 6;
	*out_pos = (byte)ae;		/* low-order byte */
	out_pos += 6;
	*out_pos = (byte)bf;		/* ditto */
	out_pos += 6;
	*out_pos = (byte)cg;		/* ditto */
	out_pos += 6;
	*out_pos = (byte)dh;		/* ditto */
	}
}
#endif
			outp += 42;
			inp++;
		   }

		/* Remove trailing 0s. */
		while ( out_end - 18 >= out )
		   {	if ( *(char *)(out_end-18) != 0 ||
		             memcmp(cmp, (char *)out_end-18, 18) != 0 )
				break;
			out_end -= 18;
		   }
		while ( out_end - 6 >= out )
		   {	if ( *(char *)(out_end-6) != 0 ||
		             memcmp(cmp, (char *)out_end-6, 6) != 0 )
				break;
			out_end -= 6;
		   }

		/* Process line */
		/* Every 6 bytes form a column of 48 dots */
		/* Horizontal skipping is in units of 1/120" */
		/* which, at 360 dpi, 3 columns of dots (18 bytes) */

		out_last = out_beg;

		if (out_beg < out_end) {
			if (!printed_something) {
				if (pdev->height == 360 * 35) {
					lnummax = lnum + 360 * 23;
					if (lnummax > pdev->height)
						lnummax = pdev->height;
				}
			}
			if (force == 0 && printed_something == 0)
				skip_count = 0;
			printed_something = 1;
			while (skip_count-- > 0) {
				/* set/execute graphics line spacing */
				/* FS C J <m> <n> = set line spacing n/360" when m is 4 */
				putc(28, prn_stream);
				putc('C', prn_stream);
				putc('J', prn_stream);
				putc(4, prn_stream);
				putc(48, prn_stream);
				if (putc(015, prn_stream) == EOF) {
					fprintf(stderr,
						"gdevbj330: disk full\n");
					status = -1;
					goto done;
				}
			}
			skip_count = 0;
		}

		++skip_count;

		while (out_beg < out_end) {
			/* Remove leading 0s. */
			while ( out_beg + 18 < out_end )
			   {	if( *cmp != 0 ||
			            memcmp(cmp, (char *)out_beg, 18) != 0 )
					break;
				out_beg += 18;
			   }

			/* Finish line if nothing is left */
			if (out_beg >= out_end) break;

			/* Skip printer forward if we ate 0's */
			count = out_beg - out_last;
		        if ( count > 0 )
			   {
				/* ESC d <n1> <n2> = move (n1+265*n2)/120 in */
			   	putc(033, prn_stream);
				putc('d', prn_stream);	/* displace to right */
				putc( (count / 18) & 0x0ff, prn_stream);
				putc( (count / 18) >> 8, prn_stream);
			   }

			/* Find next run of 0's */
			out_pos = out_beg;
			while ( out_beg + 18 < out_end )
			   {	if( *cmp == 0 &&
			            memcmp(cmp, (char *)out_beg, 18) == 0 )
					break;
				out_beg += 6;
			   }

			/* Suck up odd bits at end of line */
			if (out_beg + 18 >= out_end) out_beg = out_end;

			count = out_beg - out_pos;
			if (count <= 0) break;

			/* Avoid re-comparing the 0's we found */
			out_last = out_beg;	/* remember for skipping */
			out_beg += 18;		/* but increment pointer */

			/* Dump our bits */
			/* ESC [ g <cl> <ch> <mode> <data> =
				high resolution graphics
				cl+256*ch = 1 + bytes of data
				mode = 16 for 360x360, 48 element graphics */
			putc(033, prn_stream);
			putc('[', prn_stream);
			putc('g', prn_stream);
			putc((count + 1) & 0xff, prn_stream);
			putc((count + 1) >> 8, prn_stream);
			putc(16, prn_stream);
			fwrite((char *)out_pos, 1, count, prn_stream);
		   }
	   }

	while (force == 1 && skip_count-- > 0) {
		/* set/execute graphics line spacing */
		/* FS C J <m> <n> = set line spacing n/360" when m is 4 */
		putc(28, prn_stream);
		putc('C', prn_stream);
		putc('J', prn_stream);
		putc(4, prn_stream);
		putc(48, prn_stream);
		putc(015, prn_stream);		/* CR */
	}

	/* Reinitialize the printer ?? */
	putc(014, prn_stream);	/* form feed */

done:

#if TRN_COUNT
	fprintf(stderr,
		"total %ld, 0 %ld, 1 %ld, ints eq %ld, bytes eq %ld, not eq %ld\n",
		trn_total, trn_0, trn_1, trn_eq_i, trn_eq_b,
		trn_total - trn_0 - trn_1 - trn_eq_i - trn_eq_b);
#endif

	gs_free(in, 1, 48*line_size, "bj330_print_page(in)");
	gs_free(out, 1, 48*line_size, "bj330_print_page(out)");
	return(status);
}
