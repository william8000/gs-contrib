ghostscript drivers (unmaintained)
==================================

This repository contains some ghostscript drivers.

To use them, copy them to the gs `devices` directory and edit `devs.mak` as needed using `scsdevs.mak` as an example, or including it directly.

The drivers are based on gs drivers and inherit the same license as gs.

Devices
-------

* gdevansi.c - Characters on an ansi/vt100 terminal
* gdevbj330.c - Canon Bubble Jet BJ-330 printer
* gdevci.c - CIE CI-4000 Tri Printer Model 20
* gdevl1k.c - 256-color VGA modes with Linux and vgalib (no longer works, requires a text mode console)
* gdevlips10.c - C.Itoh LIPS 10 Laser Printer
* gdevsixel.c - vt300-class terminal with sixel graphics
* gdevtek.c - vt300-class terminal with Tektronics supporting 1024 X 768 Graph Mode with 10-bit addressing
