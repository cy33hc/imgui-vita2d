TARGET		:= imgui
TITLE		:= IMGUI0001

LIBS = -limgui_vita2d -lvita2d_sys -lc -lSceCommonDialog_stub -lSceLibKernel_stub \
	-lSceDisplay_stub -lSceGxm_stub -lSceSysmodule_stub -lSceCtrl_stub \
	-lSceTouch_stub -lm -lSceAppMgr_stub -lSceAppUtil_stub -lSceShaccCg_stub \
	-lSceSharedFb_stub -lSceDbg_stub -lSceGxmInternalForVsh_stub -lSceDmacmgr_stub \
	-lSceFios2_stub -lSceDisplayUser_stub

CPPSOURCES	:= .

CFILES	:= 
CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

PREFIX  = arm-dolce-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O2 -g -mtune=cortex-a9 -mfpu=neon
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: $(TARGET).velf
	dolce-make-fself -s $< eboot.bin
	dolce-mksfoex -s TITLE_ID=$(TITLE) "$(TARGET)" param.sfo
	dolce-make-pkg -f vpk -t app -a eboot.bin eboot.bin -a param.sfo sce_sys/param.sfo imgui.vpk

%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	dolce-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS) $(TARGET).elf.unstripped.elf $(TARGET).vpk build/eboot.bin build/sce_sys/param.sfo ./param.sfo
