.PHONY: all clean
all:


SRC := \
	WinNTSetup.cxx \
	FileUtil.cxx \
	BcdUtil.cxx

INC := \
	../wimlib/include \
	reactos/boot/environ/include \
	reactos/sdk/include/host \
	reactos/sdk/include/psdk

BIN := WinNTSetup

OBJ := $(patsubst %.cxx,%.o,$(SRC))
DEP := $(patsubst %.cxx,%.d,$(SRC))

all: $(BIN)

$(BIN): $(OBJ)
	gcc $^ -lfltk -lpthread -lstdc++ -lblkid -L../wimlib/.libs -lwim -o $@

$(OBJ): flags := $(addprefix -I,$(INC))
$(OBJ): %.o : %.cxx
	gcc $(flags) -c $< -MD -MF $(patsubst %.o,%.d,$@) -o $@

clean:
	rm -f $(BIN) $(OBJ) $(DEP)

include $(wildcard $(DEP))

