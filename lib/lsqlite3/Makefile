
ROOT_DIR = ../..

TARGET = lib_lsqlite3.a

SRC += lsqlite3.c

CFLAGS += -I$(ROOT_DIR)/lib/lua
CFLAGS += -I$(ROOT_DIR)/lib/sqlite3
CFLAGS += -DSQLITE_OMIT_LOAD_EXTENSION

OBJS = $(patsubst %c, %o, $(SRC))

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) r $@ $^

clean:
	rm -f $(TARGET) $(OBJS)

