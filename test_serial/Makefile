SOURCES += $(call rwildcard,.,*.cpp)
SOURCES += $(call rwildcard,../serial,*.cpp)
INCDIR += $(dir ($(call rwildcard,.,*.h)))
INCDIR += $(dir ($(call rwildcard,../serial,*.h)))
INCDIR += ..

TCHAIN = x86_64-w64-mingw32-

# EXT_LIBS+=SetupAPI_x86
EXT_LIBS+=setupapi
LIBDIR+=../serial

VERBOSE=yes

include ../core.mk

all: run

.PHONY: run
run: $(EXECUTABLE)
	@$(EXECUTABLE)