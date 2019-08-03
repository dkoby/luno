################################################################################
#
#
################################################################################
ROOT_DIR = ..

ifdef WINDOWS
	TARGET_W = $(ROOT_DIR)/luno.exe
	TARGET   = $(TARGET_W)
else
	TARGET_L = $(ROOT_DIR)/luno
	TARGET   = $(TARGET_L)
endif

LUA   = $(ROOT_DIR)/lib/lua/host/lua
LUAC  = $(ROOT_DIR)/lib/lua/host/luac
BIN2C = $(LUA) $(ROOT_DIR)/bin2c.lua

########################################
#
#
########################################
C_FILES += client.c
C_FILES += common.c
C_FILES += debug.c
C_FILES += http.c
C_FILES += lclient.c
C_FILES += lmromfs.c
C_FILES += lserver.c
C_FILES += main.c
C_FILES += mromfs.c
C_FILES += server.c
C_FILES += thread.c
C_FILES += token.c

C_OBJS = $(foreach obj, $(C_FILES), $(patsubst %c, %o, $(obj)))
OBJS += $(C_OBJS)

SCRIPTS += lua/init0.lua
SCRIPTS += lua/init1.lua
SCRIPTS += lua/ljson.lua
SCRIPTS += lua/process.lua
SCRIPTS += lua/util.lua

LUA_OUT = $(foreach obj, $(SCRIPTS), $(patsubst %lua, %out, $(obj)))
LUA_H   = $(foreach obj, $(LUA_OUT), $(patsubst %out, %h, $(obj)))
########################################
#
#
########################################
ifdef WINDOWS
	CFLAGS += -DWINDOWS
endif
CFLAGS += -I$(ROOT_DIR)/lib/lua
CFLAGS += -I$(ROOT_DIR)/lib/lsqlite3

LDFLAGS += -static
LDFLAGS += -L$(ROOT_DIR)/lib/lua/target
LDFLAGS += -L$(ROOT_DIR)/lib/lsqlite3
LDFLAGS += -L$(ROOT_DIR)/lib/sqlite3

LIBS += -l_lua
LIBS += -l_lsqlite3
LIBS += -l_sqlite3
ifndef WINDOWS
	LIBS += -lpthread
else
	LIBS += -lsetupapi
	LIBS += -lwsock32
	LIBS += -lws2_32
endif
LIBS += -lm

VPATH += $(ROOT_DIR)/lib/lua/target
VPATH += $(ROOT_DIR)/lib/sqlite3

########################################
#
#
########################################

.PHONY: all clean depend

all: depend $(TARGET)

$(DEPFILE): $(LUA_H) Makefile
	$(CC) $(CFLAGS) -MM $(C_FILES) > $(DEPFILE)
-include $(DEPFILE)
depend: $(DEPFILE)

%.out: %.lua
	$(LUAC) $(STRIP) -o $@ $<

%.h: %.out
	$(BIN2C) $< $@ $(patsubst lua/%, %Script, $*)

$(TARGET): $(LUA_H) $(OBJS) $(PTHREADS4W_LIB) $(LIBS_VPATH)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SZ) -A $@

clean:
	rm -f $(TARGET_L) $(TARGET_W) $(OBJS) $(DEPFILE) $(LUA_H)

