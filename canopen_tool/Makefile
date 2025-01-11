EXE_NAME=CanopenTool

INCDIR  += ../sp
INCDIR  += ../canopennode
INCDIR  += ../canopennode_driver

SOURCES += $(wildcard ../sp/*.c)
SOURCES += $(wildcard ../canopennode/*.c)
SOURCES += $(wildcard ../canopennode/301/*.c)
SOURCES += $(wildcard ../canopennode/303/*.c)
SOURCES += $(wildcard ../canopennode/304/*.c)
SOURCES += $(wildcard ../canopennode/305/*.c)
SOURCES += $(wildcard ../canopennode/309/*.c)
SOURCES += $(wildcard ../canopennode_driver/*.c)
SOURCES += main.cpp mathplot.cpp

# PPDEFS += CO_FRAME_RX_CB
# PPDEFS += CO_FRAME_TX_CB
# PPDEFS += CO_SDO_HI_SPEED_MODE

PPDEFS  += CO_USE_GLOBALS
PPDEFS += CO_CONFIG_FIFO="CO_CONFIG_FIFO_ENABLE|CO_CONFIG_FIFO_ASCII_COMMANDS|CO_CONFIG_FIFO_ASCII_DATATYPES"
PPDEFS += CO_CONFIG_EM="CO_CONFIG_EM_PRODUCER|CO_CONFIG_EM_CONSUMER|CO_CONFIG_EM_HISTORY|CO_CONFIG_EM_STATUS_BITS|CO_CONFIG_FLAG_TIMERNEXT"
PPDEFS += CO_CONFIG_GFC=0
PPDEFS += CO_CONFIG_GTW="CO_CONFIG_GTW_ASCII|CO_CONFIG_GTW_ASCII_SDO|CO_CONFIG_GTW_ASCII_NMT|CO_CONFIG_GTW_ASCII_LSS"
PPDEFS += CO_CONFIG_GTWA_COMM_BUF_SIZE=2000
PPDEFS += CO_CONFIG_GTW_BLOCK_DL_LOOP=3
PPDEFS += CO_CONFIG_HB_CONS="CO_CONFIG_HB_CONS_ENABLE|CO_CONFIG_HB_CONS_CALLBACK_MULTI|CO_CONFIG_FLAG_TIMERNEXT|CO_CONFIG_FLAG_OD_DYNAMIC"
PPDEFS += CO_CONFIG_LEDS=0
PPDEFS += CO_CONFIG_LSS="CO_CONFIG_LSS_MASTER|CO_CONFIG_LSS_SLAVE_FASTSCAN_DIRECT_RESPOND|CO_CONFIG_FLAG_CALLBACK_PRE"
PPDEFS += CO_CONFIG_NMT="CO_CONFIG_NMT_MASTER|CO_CONFIG_FLAG_TIMERNEXT"
PPDEFS += CO_CONFIG_PDO="CO_CONFIG_RPDO_ENABLE|CO_CONFIG_TPDO_ENABLE|CO_CONFIG_RPDO_TIMERS_ENABLE|CO_CONFIG_TPDO_TIMERS_ENABLE|CO_CONFIG_PDO_SYNC_ENABLE|CO_CONFIG_PDO_OD_IO_ACCESS|CO_CONFIG_FLAG_TIMERNEXT|CO_CONFIG_FLAG_OD_DYNAMIC"
PPDEFS += CO_CONFIG_SDO_CLI="CO_CONFIG_SDO_CLI_ENABLE|CO_CONFIG_SDO_CLI_SEGMENTED|CO_CONFIG_SDO_CLI_LOCAL|CO_CONFIG_FLAG_TIMERNEXT"
PPDEFS += CO_CONFIG_SDO_CLI_BUFFER_SIZE=534
PPDEFS += CO_CONFIG_SDO_SRV="CO_CONFIG_SDO_SRV_SEGMENTED|CO_CONFIG_FLAG_TIMERNEXT|CO_CONFIG_FLAG_OD_DYNAMIC"
PPDEFS += CO_CONFIG_SDO_SRV_BUFFER_SIZE=534
PPDEFS += CO_CONFIG_SRDO=0
PPDEFS += CO_CONFIG_SYNC="CO_CONFIG_SYNC_ENABLE|CO_CONFIG_SYNC_PRODUCER|CO_CONFIG_FLAG_TIMERNEXT|CO_CONFIG_FLAG_OD_DYNAMIC"
PPDEFS += CO_CONFIG_TIME="CO_CONFIG_TIME_ENABLE|CO_CONFIG_TIME_PRODUCER|CO_CONFIG_FLAG_OD_DYNAMIC"
PPDEFS += CO_CONFIG_TRACE=0

CFLAGS   += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CFLAGS   += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin -ffreestanding
CFLAGS   += $(C_FULL_FLAGS)
CFLAGS   += -Werror

CXXFLAGS += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CXXFLAGS += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin
CXXFLAGS += -fvisibility-inlines-hidden -fuse-cxa-atexit -felide-constructors 
# CXXFLAGS += -fno-rtti # causes errors on wxWidgets static build
# CXXFLAGS += $(CXX_FULL_FLAGS)
# CXXFLAGS += -Werror

ifneq (,$(findstring Windows,$(OS)))
	SOURCES += sample.rc
	LDFLAGS += -static
	EXT_LIBS += setupapi
	TCHAIN    = x86_64-w64-mingw32-
	WX_FLAGS := $(subst /mingw64/,C:/msys64/mingw64/, $(shell wx-config --cxxflags))
	WINDRES  := $(subst /mingw64/,C:/msys64/mingw64/, $(shell wx-config --rescomp))
	WX_LD    := $(subst /mingw64/lib/lib, -l, $(shell wx-config --libs))
	WX_LD    := $(subst .a, , $(WX_LD))
else
# don't do a static build!
	WX_FLAGS := $(shell wx-config --cxxflags)
	WX_LD    := $(shell wx-config --libs)
	WINDRES  := $(shell wx-config --rescomp)
endif

LDFLAGS += $(WX_LD)
FLAGS   += $(WX_FLAGS)

include ../core.mk

tests: $(EXECUTABLE)
	@echo "======================================================="
	@-sudo valgrind --tool=memcheck \
		--trace-children=yes \
		--demangle=yes \
		--log-file="${TEST_OUTPUT}.vg.out" \
		--leak-check=full \
		--show-reachable=yes \
		--run-libc-freeres=yes \
		-s \
		$< ${REDIR_OUTPUT}
	@if ! tail -1 "${TEST_OUTPUT}.vg.out" | grep -q "ERROR SUMMARY: 0 errors"; then \
		echo "==== ERROR: valgrind found errors during execution ===="; \
		tail -1 "${TEST_OUTPUT}.vg.out"; \
	else \
		echo "================ No errors found ======================"; \
	fi

run: $(EXECUTABLE)
	@$(EXECUTABLE)