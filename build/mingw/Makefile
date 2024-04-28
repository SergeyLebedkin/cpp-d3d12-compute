# phony
.PHONY: all asm clean

# global utils
ECHO = @echo
MKDIR_P = mkdir -p
RM_RF = rm -rf

# global objects path (please, DO NOT use "./" in path, f.e "./obj/app", but use ONLY "obj/app")
OBJ_PATH = .bin/obj

# c++ compiling
APP_CXX      = g++
APP_CXXFLAGS = -g -Wall -std=c++17
APP_ASMFLAGS = -masm=intel -Wall -std=c++17
APP_DEFINES  =                      \
    -D _DEBUG
APP_INCLUDES =                      \
    -I ./include
# app linking
APP_LD        = g++
APP_LDFLAGS   = -mconsole
APP_LIBRARIES = -L ./lib/x64 -l d3d12 -l d3dcompiler
# targets
APP_TARGET_PATH = .bin
APP_TARGET_NAME = $(APP_TARGET_PATH)/cpp-con-app.exe
# app sources
APP_SOURCES_PATH = src
APP_SOURCES :=                            \
	$(wildcard $(APP_SOURCES_PATH)/*.cpp)
APP_HEADERS :=                            \
	$(wildcard $(APP_SOURCES_PATH)/*.hpp)
# app objects
APP_SOURCES_OBJ := $(foreach file,$(APP_SOURCES),$(OBJ_PATH)/$(file).o)
APP_SOURCES_ASM := $(foreach file,$(APP_SOURCES),$(OBJ_PATH)/$(file).asm)

# create app
all: $(APP_TARGET_NAME)

asm: $(APP_SOURCES_ASM)

# link application
$(APP_TARGET_NAME): $(APP_SOURCES_OBJ)
	$(MKDIR_P) $(@D)
	$(APP_LD) $(APP_LDFLAGS) $^ $(APP_LIBRARIES) -o $@

# compile source code
$(APP_SOURCES_OBJ): $(APP_SOURCES) $(APP_HEADERS)
	$(MKDIR_P) $(@D)
	$(APP_CXX) $(APP_CXXFLAGS) $(APP_INCLUDES) $(APP_DEFINES) -c $(@:$(OBJ_PATH)/%.o=%) -o $@

# compile source code
$(APP_SOURCES_ASM): $(APP_SOURCES) $(APP_HEADERS)
	$(MKDIR_P) $(@D)
	$(APP_CXX) $(APP_ASMFLAGS) $(APP_INCLUDES) $(APP_DEFINES) -S $(@:$(OBJ_PATH)/%.asm=%) -o $@

# clean all
clean:
	$(RM_RF) $(APP_TARGET_NAME)
	$(RM_RF) $(OBJ_PATH)