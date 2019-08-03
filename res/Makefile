################################################################################
#
#
################################################################################
ROOT_DIR = ..

TCL       = tclsh
LUA       = $(ROOT_DIR)/lib/lua/host/lua
IMAGE_H   = $(ROOT_DIR)/src/mromfsimage.h
IMAGE     = mromfs.bin
BIN2C     = $(LUA) $(ROOT_DIR)/bin2c.lua
GENMROMFS = $(TCL) $(ROOT_DIR)/genmromfs.tcl
DIRMTIME  = $(TCL) $(ROOT_DIR)/dirmtime.tcl
########################################
#
#
########################################
.PHONY: all clean

all: dirmtime $(IMAGE_H)

.PHONY: dirmtime
dirmtime:
	$(DIRMTIME) $(RESOURCE)

$(IMAGE): $(RESOURCE)
	$(GENMROMFS) $(RESOURCE) $@

$(IMAGE_H): $(IMAGE)
	$(BIN2C) $< $@ mromfsimageData

clean:
	rm -f $(IMAGE_H) $(IMAGE)

