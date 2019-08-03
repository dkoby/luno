################################################################################
#
#
################################################################################
.PHONY: all

export RESOURCE = example
########################################
#
#
########################################
ifdef WINDOWS
include win.mk
endif

export SH = /bin/bash
export CC = $(CROSS_COMPILE)gcc
export LD = $(CROSS_COMPILE)gcc
export AR = $(CROSS_COMPILE)ar
export SZ = $(CROSS_COMPILE)size

export CC_HOST = gcc
export LD_HOST = gcc

#
# CFLAGS
#
ifdef DEBUG
    CFLAGS += -g
    CFLAGS += -Wextra
	CFLAGS += -O0
else
	ifdef WINDOWS
		CFLAGS += -O0
	else
		CFLAGS += -O2
	endif
endif
CFLAGS += -Wall
export CFLAGS

#
# LDFLAGS
#
ifdef DEBUG
    LDFLAGS += -v
else
    LDFLAGS += -s
endif
export LDFLAGS

export DEPFILE = depfile.mk

-include custom.mk
########################################
#
#
########################################
DIRS += lib/lua
DIRS += lib/sqlite3
DIRS += lib/lsqlite3
DIRS += res
DIRS += src

.PHONY: all clean
all:
	$(SH) foreach.sh $@ $(DIRS)

clean:
	$(SH) foreach.sh $@ $(DIRS)

clean-dist: clean
	rm -rf database.db .fossil-settings tags session.vim .fslckout

