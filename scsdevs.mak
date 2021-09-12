### ----------------- SCS Devices ----------------- ###

# Before gs3.28, uncomment the following line
# SETDEV=$(SHP)gssetdev
# Before gs5, uncomment the following line
# GLOBJ=./
# Before gs5.6, uncomment the following lines
# DD=./
# GLSRC=./
# Before gs9.09, use GLOBJ for DEVOBJ, GLCC for DEVCC, GLO_ for DEVO_, GLSRC for DEVSRC


### ----------------- The Canon BubbleJet BJ330 device ----------------- ###

BJ330=$(DEVOBJ)gdevbj330.$(OBJ)

$(DEVOBJ)gdevbj330.$(OBJ): $(DEVSRC)gdevbj330.c $(PDEVH)
	$(DEVCC) $(DEVO_)gdevbj330.$(OBJ) $(C_) $(DEVSRC)gdevbj330.c

# All of these are the same device with different page sizes
# l = 8X11 letter, w = 13X11 wide, d = 13X22, t = 13X33, b = 13X35

bj330l_=$(BJ330)
$(DD)bj330l.dev: $(bj330l_)
	$(SETDEV) $(DD)bj330l $(bj330l_)

bj330w_=$(BJ330)
$(DD)bj330w.dev: $(bj330w_)
	$(SETDEV) $(DD)bj330w $(bj330w_)

bj330d_=$(BJ330)
$(DD)bj330d.dev: $(bj330d_)
	$(SETDEV) $(DD)bj330d $(bj330d_)

bj330t_=$(BJ330)
$(DD)bj330t.dev: $(bj330t_)
	$(SETDEV) $(DD)bj330t $(bj330t_)

bj330b_=$(BJ330)
$(DD)bj330b.dev: $(bj330b_)
	$(SETDEV) $(DD)bj330b $(bj330b_)

###### ----------------------- The ansi device ------------------------ ######

ansi_=$(DEVOBJ)gdevansi.$(OBJ)
$(DD)ansi.dev: $(ansi_)
	$(SETDEV) $(DD)ansi $(ansi_)

$(DEVOBJ)gdevansi.$(OBJ): $(DEVSRC)gdevansi.c $(GDEV)
	$(DEVCC) $(DEVO_)gdevansi.$(OBJ) $(C_) $(DEVSRC)gdevansi.c

###### ----------------------- The sixel device --------------------- ######

# Use sixel with a vt330, sixelk with kermit

sixel_=$(DEVOBJ)gdevsixel.$(OBJ)
$(DD)sixel.dev: $(sixel_)
	$(SETDEV) $(DD)sixel $(sixel_)

sixelk_=$(DEVOBJ)gdevsixel.$(OBJ)
$(DD)sixelk.dev: $(sixelk_)
	$(SETDEV) $(DD)sixelk $(sixelk_)

$(DEVOBJ)gdevsixel.$(OBJ): $(DEVSRC)gdevsixel.c $(GDEV)
	$(DEVCC) $(DEVO_)gdevsixel.$(OBJ) $(C_) $(DEVSRC)gdevsixel.c

###### ----------------------- The tek device ----------------------- ######

tek_=$(DEVOBJ)gdevtek.$(OBJ)
$(DD)tek.dev: $(tek_)
	$(SETDEV) $(DD)tek $(tek_)

$(DEVOBJ)gdevtek.$(OBJ): $(DEVSRC)gdevtek.c $(GDEV)
	$(DEVCC) $(DEVO_)gdevtek.$(OBJ) $(C_) $(DEVSRC)gdevtek.c

###### ----------------------- The ci4000 device -------------------- ######

ci_=$(DEVOBJ)gdevci.$(OBJ)

$(DD)ci.dev: $(ci_)
	$(SETDEV) $(DD)ci $(ci_)

$(DD)cib.dev: $(ci_)
	$(SETDEV) $(DD)cib $(ci_)

$(DD)cilq.dev: $(ci_)
	$(SETDEV) $(DD)cilq $(ci_)

$(DD)cilqb.dev: $(ci_)
	$(SETDEV) $(DD)cilqb $(ci_)

$(DEVOBJ)gdevci.$(OBJ): $(DEVSRC)gdevci.c $(GDEV)
	$(DEVCC) $(DEVO_)gdevci.$(OBJ) $(C_) $(DEVSRC)gdevci.c

###### ---------------------- The lips10 device --------------------- ######

lips10_=$(DEVOBJ)gdevlips10.$(OBJ)
$(DD)lips10.dev: $(lips10_)
	$(SETDEV) $(DD)lips10 $(lips10_)

$(DEVOBJ)gdevlips10.$(OBJ): $(DEVSRC)gdevlips10.c $(GDEV)
	$(DEVCC) $(DEVO_)gdevlips10.$(OBJ) $(C_) $(DEVSRC)gdevlips10.c

###### -------------------- The linux l1k device ------------------- ######

lvga1k_=$(DEVOBJ)gdevl1k.$(OBJ)
$(DD)lvga1k.dev: $(lvga1k_)
	$(SETDEV) $(DD)lvga1k $(lvga1k_)
	$(ADDMOD) $(DD)lvga1k -lib vga vgagl

$(DEVOBJ)gdevl1k.$(OBJ): $(DEVSRC)gdevl1k.c $(GDEV) $(MAKEFILE) $(gxxfont_h)
	$(DEVCC) $(DEVO_)gdevl1k.$(OBJ) $(C_) $(DEVSRC)gdevl1k.c

###### ----------------------- The gif device ---------------------- ######

gif8_=$(DEVOBJ)gdevgif.$(OBJ)
$(DD)gif8.dev: $(gif8_)
	$(SETDEV) $(DD)gif8 $(gif8_)

gifmono_=$(DEVOBJ)gdevgif.$(OBJ)
$(DD)gifmono.dev: $(gifmono_)
	$(SETDEV) $(DD)gifmono $(gifmono_)

$(DEVOBJ)gdevgif.$(OBJ): $(DEVSRC)gdevgif.c $(GDEV)
	$(DEVCC) $(DEVO_)gdevgif.$(OBJ) $(C_) $(DEVSRC)gdevgif.c

### ----------------- End of SCS Devices ----------------- ###

