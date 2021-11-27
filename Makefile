.PHONY: all clean
all:

CXX_SRC := \
	WinNTSetup.cxx

C_SRC := \
	FileUtil.c \
	BcdUtil.c

C_INC := \
	reactos/boot/environ/include \
	reactos/sdk/include/host \
	reactos/sdk/include/psdk

BIN := WinNTSetup

CXX_OBJ := $(patsubst %.cxx,%.o,$(CXX_SRC))
CXX_DEP := $(patsubst %.cxx,%.d,$(CXX_SRC))
C_OBJ := $(patsubst %.c,%.o,$(C_SRC))
C_DEP := $(patsubst %.c,%.d,$(C_SRC))

all: $(BIN)

$(BIN): $(CXX_OBJ) $(C_OBJ)
	gcc $^ -lfltk -lpthread -lstdc++ -lblkid -o $@

$(CXX_OBJ): flags := $(addprefix -I,$(CXX_INC))
$(CXX_OBJ): %.o : %.cxx
	g++ $(flags) -c $< -MD -MF $(patsubst %.o,%.d,$@) -o $@

$(C_OBJ): flags := $(addprefix -I,$(C_INC))
$(C_OBJ): %.o : %.c
	gcc $(flags) -c $< -MD -MF $(patsubst %.o,%.d,$@) -o $@

clean:
	rm -f $(BIN) $(CXX_OBJ) $(C_OBJ) $(CXX_DEP) $(C_DEP)

include $(wildcard $(CXX_DEP) $(C_DEP))

