.PHONY: all
all:

SRC := \
	BootRecLinux.c \
	FileUtil.c

INC := \
	reactos/boot/environ/include \
	reactos/sdk/include/host \
	reactos/sdk/include/psdk

BIN := BootRec

OBJ := $(patsubst %.c,%.o,$(SRC))

all: $(BIN)

$(BIN): $(OBJ)
	gcc $^ -o $@

$(OBJ): flags := $(addprefix -I,$(INC))
$(OBJ): %.o : %.c
	gcc $(flags) -c $< -o $@

