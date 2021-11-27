.PHONY: all clean
all:

SRC := \
	main.c \
	FileUtil.c \
	BcdUtil.c

INC := \
	reactos/boot/environ/include \
	reactos/sdk/include/host \
	reactos/sdk/include/psdk

BIN := BootRec

OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

all: $(BIN)

$(BIN): $(OBJ)
	gcc $^ -o $@

$(OBJ): flags := $(addprefix -I,$(INC))
$(OBJ): %.o : %.c
	gcc $(flags) -c $< -MD -MF $(patsubst %.o,%.d,$@) -o $@

clean:
	rm -f $(BIN) $(OBJ) $(DEP)

include $(wildcard $(DEP))
