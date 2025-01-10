# TCHAIN = x86_64-w64-mingw32-

INCDIR  += ..
INCDIR  += ../sp
SOURCES += ../sp/sp_enum.c
SOURCES += ../sp/sp_unix.c
SOURCES += ../sp/sp_win.c

SOURCES += main.c

# EXT_LIBS += setupapi

LDFLAGS += -static

# DBG_OPTS = -gdwarf-2 -ggdb -g

include ../core.mk

run: $(EXECUTABLE)
	@$(EXECUTABLE)