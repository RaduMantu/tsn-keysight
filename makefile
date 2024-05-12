#important directories
SRC = src
BIN = bin
OBJ = obj
INC = include

# compilation related parameters
CC      = clang
CFLAGS  = -I$(INC) -O2 -gdwarf-5
LDFLAGS =

# identify sources & create object targets
SOURCES = $(wildcard $(SRC)/*.c)
OBJECTS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

# prevent deletion of intermediary files / directories
.SECONDARY:

# phony targets
.PHONY: build clean

# default target
build: $(BIN)/tsn

# ephemeral directory generation
%/:
	@mkdir -p $@

# final binary generation
$(BIN)/tsn: $(OBJECTS) | $(BIN)/
	$(CC) -o $@ $^ $(LDFLAGS)

# object generation
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)/
	$(CC) -c $(CFLAGS) -o $@ $^

# cleanup
clean:
	@rm -rf $(BIN) $(OBJ)

