SOURCES += $(call rwildcard,.,*.cpp)
SOURCES += $(call rwildcard,../serial,*.cpp)
INCDIR += $(dir ($(call rwildcard,.,*.h)))
INCDIR += $(dir ($(call rwildcard,../serial,*.h)))
INCDIR += ..

MINGW=1

EXT_LIBS+=setupapi

ifneq ($(strip $(MINGW)),) 
	TCHAIN = x86_64-w64-mingw32-
	EXT_LIBS+=setupapi
# LDFLAGS+=-Wl,-Bstatic
endif

include ../core.mk

# all: run

# .PHONY: run
run: $(EXECUTABLE)
	@$(EXECUTABLE)