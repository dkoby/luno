################################################################################
#
#
################################################################################
.PHONY: all

########################################
#
#
########################################
CFLAGS += -DLUA_COMPAT_5_2

CFLAGS_HOST += -DLUA_USE_LINUX
CFLAGS_HOST += -DLUA_USE_DLOPEN

ifdef WINDOWS
	CFLAGS_TARGET += -DLUA_USE_WINDOWS
else
	CFLAGS_TARGET += -DLUA_USE_LINUX
endif

########################################
#
#
########################################
C_FILES += lapi.c
C_FILES += lauxlib.c
C_FILES += lbaselib.c
C_FILES += lbitlib.c
C_FILES += lcode.c
C_FILES += lcorolib.c
C_FILES += lctype.c
C_FILES += ldblib.c
C_FILES += ldebug.c
C_FILES += ldo.c
C_FILES += ldump.c
C_FILES += lfunc.c
C_FILES += lgc.c
C_FILES += linit.c
C_FILES += liolib.c
C_FILES += llex.c
C_FILES += lmathlib.c
C_FILES += lmem.c
C_FILES += loadlib.c
C_FILES += lobject.c
C_FILES += lopcodes.c
C_FILES += loslib.c
C_FILES += lparser.c
C_FILES += lstate.c
C_FILES += lstring.c
C_FILES += lstrlib.c
C_FILES += ltable.c
C_FILES += ltablib.c
C_FILES += ltm.c
C_FILES += lundump.c
C_FILES += lutf8lib.c
C_FILES += lvm.c
C_FILES += lzio.c

OBJS_HOST   = $(foreach obj,$(C_FILES) ,$(patsubst %c, host/%o, $(obj)))
OBJS_TARGET = $(foreach obj,$(C_FILES) ,$(patsubst %c, target/%o, $(obj)))
########################################
#
#
########################################
.PHONY: clean

all: host target host/lua host/lua host/luac target/lib_lua.a

host:
	mkdir -p host
target:
	mkdir -p target

########################################
#
#
########################################
host/%o: %c
	$(CC_HOST) $(CFLAGS) $(CFLAGS_HOST) -c -o $@ $^

host/lua: $(OBJS_HOST) host/lua.o
	$(LD_HOST) $(LDFLAGS) -o $@ $^ -ldl -lm
host/luac: $(OBJS_HOST) host/luac.o
	$(LD_HOST) $(LDFLAGS) -o $@ $^ -ldl -lm

########################################
#
#
########################################
target/%o: %c
	$(CC) $(CFLAGS) $(CFLAGS_TARGET) -c -o $@ $^

target/lib_lua.a: $(OBJS_TARGET)
	$(AR) r $@ $^

clean:
	rm -rf host target

