
TARGET = lib_sqlite3.a

SRC += sqlite3.c

CFLAGS += -DSQLITE_OMIT_LOAD_EXTENSION

OBJS = $(patsubst %c, %o, $(SRC))

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) r $@ $^

clean:
	rm -f $(TARGET) $(OBJS)

