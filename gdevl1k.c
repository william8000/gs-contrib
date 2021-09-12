
/* Copyright (C) 1989, 1990, 1991, 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gdevl1k.c */
/* Ghostscript driver for 256-color VGA modes with Linux and vgalib */
/* This Driver was derived from the gdevl256.
   New features:
   1. support change resolution from command line ( using -rRES or
   -rRESXxRESY.
   2. support scrolling. You can use arrow keys to scroll the image if
   the image size is larger than the screen size.
   3. you can change vga mode while you\'re preview the output page.

   New option, you can use "-DNO_SYNC" to tell the driver only output
   when a page is completed.

   It works with my trident 8900c card. I\'m not sure about other
   video cards.
   Dong Liu <dliu@ace.njit.edu> 12/16/94
   
 */
#include "gx.h"
#include "gxdevice.h"
#include "gserrors.h"
#include "ghost.h"
#include "store.h"
#include "dstack.h"
#include "interp.h"
#include "idict.h"

#include <errno.h>
#include <vga.h>
#include <vgagl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <linux/vt.h>

/* The color map for dynamically assignable colors. */
#define first_dc_index 64
private int next_dc_index;

#define dc_hash_size 293                    /* prime, >num_dc */
typedef struct {
    ushort rgb, index;
} dc_entry;

private dc_entry dynamic_colors[dc_hash_size + 1];

/* The device descriptor */
typedef struct gx_device_lvga1024 {
    gx_device_common;
    GraphicsContext screen_gc;
    GraphicsContext virtual_gc;
    char *lvga1024_vbuf;
    void *lvga1024_font;
    int lvga1024_scr_width, lvga1024_scr_height;
} gx_device_lvga1024;


#define lvga1024dev ((gx_device_lvga1024 *)dev)

private dev_proc_open_device (lvga1024_open);
private dev_proc_close_device (lvga1024_close);
private dev_proc_sync_output (lvga1024_sync_output);
private dev_proc_output_page (lvga1024_output_page);
private dev_proc_map_rgb_color (lvga1024_map_rgb_color);
private dev_proc_map_color_rgb (lvga1024_map_color_rgb);
private dev_proc_fill_rectangle (lvga1024_fill_rectangle);
private dev_proc_tile_rectangle (lvga1024_tile_rectangle);
private dev_proc_copy_mono (lvga1024_copy_mono);
private dev_proc_copy_color (lvga1024_copy_color);
private dev_proc_draw_line (lvga1024_draw_line);

private gx_device_procs lvga1024_procs =
{
    lvga1024_open,
    NULL, 	              /* get_initial_matrix */
    lvga1024_sync_output,
    lvga1024_output_page,
    lvga1024_close,
    lvga1024_map_rgb_color,
    lvga1024_map_color_rgb,
    lvga1024_fill_rectangle,
    lvga1024_tile_rectangle,
    lvga1024_copy_mono,
    lvga1024_copy_color,
    lvga1024_draw_line
};

#define LVGA_XDPI 86
#define LVGA_YDPI 86
#define PAGE_WIDTH 850
#define PAGE_HEIGHT 800
static lvga1024_width = PAGE_WIDTH;
static lvga1024_height = PAGE_HEIGHT;
gx_device_lvga1024 gs_lvga1k_device =
{
    std_device_color_body(gx_device_lvga1024, &lvga1024_procs, "lvga1024",
			  (PAGE_WIDTH*LVGA_XDPI/72), /* width */
                          (PAGE_HEIGHT*LVGA_YDPI/72), /*height */
			  LVGA_XDPI, LVGA_YDPI,  
			  /*dci_color(*/8, 31, 4/*)*/)
};

#undef min
#undef abs
#define min(x,y) ((x<y)?x:y)
#define abs(x)   ((x<0)?(-x):(x))

private int do_expensive_sync;

/* Open the LINUX driver for graphics mode */
int
lvga1024_open (gx_device * d)
{
    gx_device_lvga1024 *dev = (gx_device_lvga1024 *)d;
    int vgamode;
    ref value;
    ref *exp;

#ifdef 0
    /* this dosen\'t work */
    if (dict_find_string(systemdict, "DETACH", &exp)==1) {
	/* detach to another avaiable vt */
	long a_vt;
	int vt_fd = open("/dev/console", O_RDONLY);
	if (vt_fd > 0) {
	    ioctl(vt_fd, VT_OPENQRY, &a_vt);
	    if(a_vt && (a_vt < 10)) {
		char buf[50];
		sprintf(buf, "Switching to vt %d\n", a_vt);
		write(2,buf,strlen(buf));
		ioctl(vt_fd, VT_ACTIVATE, a_vt);
		close(vt_fd);
	    }
	}
    }
#endif
    
    seteuid(0);
    vga_init();

    vgamode = vga_getdefaultmode();
    if(vgamode == -1)
	vgamode = G640x480x256;
    if (!vga_hasmode(vgamode)) 
	return -1;

    /* Make NOPAUSE TRUE, because we don\'t want showpage to wait for */
    /* a RETURN */
    make_true(&value);
    initial_enter_name("NOPAUSE",&value);
    
    /* see if NO_SYNC defined */
    if ((dict_find_string(systemdict, "NO_SYNC", &exp))==1)
	do_expensive_sync = 0;
    else
	do_expensive_sync = 1;
    
    /* find the x,y resolutions in system dictory   */
    if ((dict_find_string(systemdict, "DEVICEXRESOLUTION", &exp))==1) 
	dev->x_pixels_per_inch = exp->value.realval;
    else
	dev->x_pixels_per_inch = LVGA_XDPI;
    if ((dict_find_string(systemdict, "DEVICEYRESOLUTION", &exp))==1) 
	dev->y_pixels_per_inch = exp->value.realval;
    else
	 dev->y_pixels_per_inch = LVGA_YDPI;
    
    lvga1024_width = dev->x_pixels_per_inch*PAGE_WIDTH/72;
    lvga1024_height = dev->y_pixels_per_inch*PAGE_HEIGHT/72;
    dev->width = lvga1024_width;
    dev->height = lvga1024_height;
    dev->PageSize[0] = (dev->width*72.0)/dev->x_pixels_per_inch;
    dev->PageSize[1] = (dev->height*72.0)/dev->y_pixels_per_inch;

    vga_setmode (vgamode);
    gl_setcontextvga (vgamode);
    gl_getcontext( &(dev->screen_gc));
    if(!(dev -> lvga1024_vbuf =
	 gs_malloc(lvga1024_width*lvga1024_height,
		   1, "lvga1024_open")))
	return_error(gs_error_VMerror);
    gl_setcontextvirtual(lvga1024_width,lvga1024_height,1,8,
			 (void *)dev->lvga1024_vbuf);
    gl_getcontext( &(dev->virtual_gc));
    dev -> lvga1024_font = gs_malloc(256 * 8 * 8 *
				     BYTESPERPIXEL, 1, "lvga1024_open"); 
    gl_expandfont(8, 8, 63, gl_font8x8, dev -> lvga1024_font);
    gl_setfont(8, 8, dev -> lvga1024_font);
    gl_setwritemode(WRITEMODE_OVERWRITE);

    dev->lvga1024_scr_width = vga_getxdim();
    dev->lvga1024_scr_height = vga_getydim();
    {
      int c;

      for (c = 0; c < 64; c++) {
         static const byte c2[10] =
         {0, 42, 0, 0, 0, 0, 0, 0, 21, 63};

         gl_setpalettecolor (c, c2[(c >> 2) & 9], c2[(c >> 1) & 9], c2[c & 9]);
      }
    }
    /* Initialize the dynamic color table. */
    memset (dynamic_colors, 0, (dc_hash_size + 1) * sizeof (dc_entry));
    next_dc_index = first_dc_index;

    /* enable it run in background */
    vga_runinbackground(1);
    return 0;
}

/* Close the LINUX driver */
int
lvga1024_close (gx_device * d)
{
    gx_device_lvga1024 *dev = (gx_device_lvga1024 *)d;
    gl_freecontext(&(dev->virtual_gc));
    gs_free(dev->lvga1024_vbuf, lvga1024_width*lvga1024_height,1, "lvga1024_open");
    gs_free(dev->lvga1024_font, 256 * 8 * 8 *
	    BYTESPERPIXEL, 1, "lvga1024_open");
    while (!vga_oktowrite())
	pause();
    vga_setmode (TEXT);
    return 0;
}

/* use for scrolling image, screenx, screeny are the offset of the */
/* real image at  the screen point (0,0) */

static int screenx, screeny ;

static void
lvga1024_update(gx_device_lvga1024 *dev)
{
    if(lvga1024_height < dev->lvga1024_scr_height)
	screeny = 0;
    else if (screeny + dev->lvga1024_scr_height > lvga1024_height)
	screeny = lvga1024_height - dev->lvga1024_scr_height - 1;
    if(lvga1024_width < dev->lvga1024_scr_width )
	screenx = 0;
    else  if (screenx + dev->lvga1024_scr_width > lvga1024_width)
	screenx = lvga1024_width - dev->lvga1024_scr_width - 1;

    /* wait for we\'er the active VC */
    while (!vga_oktowrite())
	pause();
    vga_lockvc();
    gl_putboxpart(0, 0, min(lvga1024_width,dev->lvga1024_scr_width),
		  min(lvga1024_height,dev->lvga1024_scr_height),
		  lvga1024_width, lvga1024_height,
		  dev->lvga1024_vbuf,screenx, screeny);
    vga_unlockvc();

}

int
lvga1024_sync_output(gx_device *d)
{
    gx_device_lvga1024 *dev = (gx_device_lvga1024 *)d;
    if (!vga_oktowrite())
	return 0;
    gl_setcontext( &(dev->screen_gc));
    lvga1024_update(dev);
    gl_setcontext(&(dev->virtual_gc));
    return 0;
}

/* do sync beteen virtual_gc and the screen_gc, the region should be */
/* update is located at (x,y) with witdh w, height h */

static void
lvga1024_expensive_real_sync_output(gx_device_lvga1024 *dev,
			       int x, int y,int w, int h)
{
    int a, b, c, d;
    if ( !do_expensive_sync)
	return;

    a = y; b = y + h;
    c = screeny, d = min(lvga1024_height,screeny+ dev -> lvga1024_scr_height);
    if (( b < c)||( a > d))
	return;

    y = (a > c) ? a : c;
    h = (b > d) ? d - y : b - y ;

    a = x; b = x + w;
    c = screenx, d = min(lvga1024_width,screenx + dev -> lvga1024_scr_width);
    if ((b < c)||(a > d)) {
	return;
    }
    x = (a > c) ? a : c;
    w = (b > d) ? d - x : b - x ;

    if (!vga_oktowrite())
	return;
    vga_lockvc();
    gl_setcontext( &(dev->screen_gc));
    gl_putboxpart(x-screenx, y-screeny, min(lvga1024_width,w),
		  min(lvga1024_height,h), lvga1024_width, lvga1024_height,
		  dev->lvga1024_vbuf,x, y);
    gl_setcontext(&(dev->virtual_gc));
    vga_unlockvc();
}
    
int
lvga1024_output_page(gx_device *d, int num_copies, int flush)
{
    gx_device_lvga1024 *dev = (gx_device_lvga1024 *)d;
    int ch;
    static mode[4] = {5,10,11,12};
    static hide;
    int step = dev->lvga1024_scr_height/8;
 
    gl_setcontext( &(dev->screen_gc));

    for(;;) {
	lvga1024_update(dev);
	if(hide==0)
	    gl_write(0,dev->lvga1024_scr_height-8,
		     "Arrows (H)ide (1-4)Mode SPC exit");
cont:
	switch ((ch = vga_getch())) {
	case 'h': 
	case 'H':
	    hide = (hide == 0);
	    break;
	case '1'...'4':		/* change mode */
	    if (vga_hasmode(mode[ch - '1'])) {
		int pal[3*256];
		while(!vga_oktowrite())
		    pause();
		vga_lockvc();
		vga_getpalvec(0,255,pal);
		vga_setmode(mode[ch - '1']);
		gl_setcontextvga (mode[ch - '1']);
		gl_getcontext( &(dev->screen_gc));
		dev->lvga1024_scr_width = vga_getxdim();
		dev->lvga1024_scr_height = vga_getydim();
		vga_setpalvec(0,255,pal);
		vga_unlockvc();
	    }
	    break;
	case 27 :	/* cursor keys (shift picture) */
				/* scrolling the image */
	    {
		int k;
		k = vga_getch();
		if (k == 91 || k == 79) {
		    switch (vga_getch()) {
		    case 66:		/* scroll up */
			if ( screeny + dev->lvga1024_scr_height <
			     lvga1024_height) {
			    screeny += step;
			    break;
			} else
			    goto cont;
		    case 65:              /* down */
			if (screeny >= step) {
			    screeny -= step;
			    break;
			} else
			    goto cont;
		    case 67:             /* left */
			if ( screenx + dev->lvga1024_scr_width <
			     lvga1024_width) {
			    screenx += step;
			    break;
			} else
			    goto cont;
		    case 68:            /* right */
			if (screenx >= step) {
			    screenx -= step;
			    break;
			} else
			    goto cont;
		    }
		}
		break;
	    }
	case ' ':
	case '\n':
	case '\r':
	    gl_setcontext(&(dev->virtual_gc));
	    return 0;
	}
    }
}
/* Map a r-g-b color to a palette index. */
/* The first 64 entries of the color map are set */
/* for compatibility with the older display modes: */
/* these are indexed as 0.0.R0.G0.B0.R1.G1.B1. */
gx_color_index
lvga1024_map_rgb_color (gx_device * dev, gx_color_value r, gx_color_value g,
                      gx_color_value b)
{
#define cv_bits(v,n) (v >> (gx_color_value_bits - n))
   ushort r5 = cv_bits (r, 5), g5 = cv_bits (g, 5), b5 = cv_bits (b, 5);
   static const byte cube_bits[32] =
   {0, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    8, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    1, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    9
   };
   uint cx = ((uint) cube_bits[r5] << 2) + ((uint) cube_bits[g5] << 1) +
   (uint) cube_bits[b5];
   ushort rgb;
   register dc_entry _ds *pdc;

   /* Check for a color on the cube. */
   if (cx < 64)
      return (gx_color_index) cx;
   /* Not on the cube, check the dynamic color table. */
   rgb = (r5 << 10) + (g5 << 5) + b5;
   for (pdc = &dynamic_colors[rgb % dc_hash_size]; pdc->rgb != 0; pdc++) {
      if (pdc->rgb == rgb)
         return (gx_color_index) (pdc->index);
   }
   if (pdc == &dynamic_colors[dc_hash_size]) {  /* Wraparound */
      for (pdc = &dynamic_colors[0]; pdc->rgb != 0; pdc++) {
         if (pdc->rgb == rgb)
            return (gx_color_index) (pdc->index);
      }
   }
   if (next_dc_index == 256) {  /* No space left, report failure. */
       return gx_no_color_index;
   }
   /* Not on the cube, and not in the dynamic table. */
   /* Put in the dynamic table if space available. */
   {
      int i = next_dc_index++;

      pdc->rgb = rgb;
      pdc->index = i;
      while(!vga_oktowrite())
	  pause();
      vga_lockvc();
      gl_setpalettecolor (i,cv_bits (r, 6), cv_bits (g, 6), cv_bits
			  (b, 6));
      vga_unlockvc();
      return (gx_color_index) i;
   }
}

int
lvga1024_map_color_rgb (gx_device * dev, gx_color_index color,
                      unsigned short prgb[3])
{
/*   gl_getpalettecolor (color,(int *)&prgb[0],(int *)&prgb[1],(int *)&prgb[2]);*/
   prgb[0]=gx_max_color_value;
   prgb[1]=gx_max_color_value;
   prgb[2]=gx_max_color_value;
   return 0;
}

/* Copy a monochrome bitmap.  The colors are given explicitly. */
/* Color = gx_no_color_index means transparent (no effect on the image). */
int
lvga1024_copy_mono (gx_device * dev,
                  const byte * base, int sourcex, int raster, gx_bitmap_id id,
                  int x, int y, int w, int h,
                  gx_color_index zero, gx_color_index one)
{
   const byte *ptr_line = base + (sourcex >> 3);
   int left_bit = 0x80 >> (sourcex & 7);
   int dest_y = y, end_x = x + w;
   int invert = 0;
   int color;

   fit_copy (dev,base, sourcex, raster, id, x, y, w, h);
   if (zero == gx_no_color_index) {
      if (one == gx_no_color_index)
         return 0;
      color = (int) one;
   }
   else {
      if (one == gx_no_color_index) {
         color = (int) zero;
         invert = -1;
      }
      else {                    /* Pre-clear the rectangle to zero */
         gl_fillbox (x, y, w, h, 0);
	 lvga1024_expensive_real_sync_output((gx_device_lvga1024
					      *)dev,x,y,w,h); 
         color = (int) one;
      }
   }
   while (h--) {                /* for each line */
      const byte *ptr_source = ptr_line;
      register int dest_x = x;
      register int bit = left_bit;

      while (dest_x < end_x) {  /* for each bit in the line */
         if ((*ptr_source ^ invert) & bit) {
            gl_setpixel (dest_x, dest_y, color);
	    lvga1024_expensive_real_sync_output((gx_device_lvga1024
						 *)dev,dest_x,dest_y,1,1); 
         }
         dest_x++;
         if ((bit >>= 1) == 0)
            bit = 0x80, ptr_source++;
      }
      dest_y++;
      ptr_line += raster;
   }
   return 0;
}

/* Copy a color pixel map.  This is just like a bitmap, except that */
/* each pixel takes 4 bits instead of 1 when device driver has color. */
int
lvga1024_copy_color (gx_device * dev,
                   const byte * base, int sourcex, int raster, gx_bitmap_id id,
                   int x, int y, int w, int h)
{
   fit_copy (dev,base, sourcex, raster, id, x, y, w, h);
   if (gx_device_has_color (dev)) { /* color device, four bits per pixel */
      const byte *line = base + sourcex;

      gl_putbox (x, y, w, h, line);
      lvga1024_expensive_real_sync_output((gx_device_lvga1024 *)dev,x,y,w,h);
   }
   else {                       /* monochrome device: one bit per pixel */
      /* bit map is the same as lvga1024_copy_mono: one bit per pixel */
      lvga1024_copy_mono (dev, base, sourcex, raster, id, x, y, w, h,
                        (gx_color_index) 0, (gx_color_index) 255);
   }
   return 0;
}

/* Fill a rectangle. */
int
lvga1024_fill_rectangle (gx_device * dev, int x, int y, int w, int h,
                       gx_color_index color)
{
   fit_fill (dev,x,y,w,h);
   gl_fillbox (x, y, w, h, color);
   lvga1024_expensive_real_sync_output((gx_device_lvga1024 *)dev, x, y, w, h);
   return 0;
}

/* Tile a rectangle.  If neither color is transparent, */
/* pre-clear the rectangle to color0 and just tile with color1. */
/* This is faster because of how lvga1024_copy_mono is implemented. */
/* Note that this also does the right thing for colored tiles. */
int
lvga1024_tile_rectangle (gx_device * dev, const gx_tile_bitmap * tile,
                       int x, int y, int w, int h, gx_color_index czero, gx_color_index cone,
                       int px, int py)
{
   if (czero != gx_no_color_index && cone != gx_no_color_index) {
      lvga1024_fill_rectangle (dev, x, y, w, h, czero);
      czero = gx_no_color_index;
   }
   return gx_default_tile_rectangle (dev, tile, x, y, w, h, czero, cone, px, py);
}


/* Draw a line */
int
lvga1024_draw_line (gx_device * dev, int x0, int y0, int x1, int y1,
                  gx_color_index color)
{
   
    gl_line (x0, y0, x1, y1, color);
    lvga1024_expensive_real_sync_output((gx_device_lvga1024 *)dev,
					min(x0,x1), min(y0,y1), 
					abs(x1-x0)+1, abs(y1-y0)+1);
   return 0;
}
