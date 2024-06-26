#important directories
SRC = src
BIN = bin
OBJ = obj
INC = include

# system base frequency [kHz]
BASE_FREQ = $(shell cat /sys/devices/system/cpu/cpu0/cpufreq/base_frequency)

# compilation related parameters
CC      = clang
CFLAGS  = -I$(INC) -O2 -gdwarf-5 -DBASE_FREQ=$(BASE_FREQ)
LDFLAGS =

# debug / test parameters
CFLAGS += $(if $(NODEBUG),-DDEBUG_EN=0)
CFLAGS += $(if $(DUMMY_MODE),-DDUMMY_ENABLED=$(DUMMY_MODE))

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

