TARGET		:= libimgui_vita2d

LIBS = -lvita2d -lc -lSceCommonDialog_stub -lSceLibKernel_stub \
	-lSceDisplay_stub -lSceGxm_stub -lSceSysmodule_stub -lSceCtrl_stub \
	-lSceTouch_stub -lm -lSceAppMgr_stub -lSceAppUtil_stub  -lSceShaccCg_stub

CPPSOURCES	:= .
ASMSOURCES := .
ASMFILES	:= $(foreach dir,$(ASMSOURCES), $(wildcard $(dir)/*.S))
CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(ASMFILES:.S=.o) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O2 -g -mtune=cortex-a9 -mfpu=neon -ftree-vectorize
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)
AR      = $(PREFIX)-gcc-ar

all: $(TARGET).a

$(TARGET).a: $(OBJS)
	$(AR) -rc $@ $^

install: $(TARGET).a
	@mkdir -p $(VITASDK)/$(PREFIX)/lib/
	cp $(TARGET).a $(VITASDK)/$(PREFIX)/lib/
	@mkdir -p $(VITASDK)/$(PREFIX)/include/imgui_vita2d
	cp imgui_vita2d/*.h $(VITASDK)/$(PREFIX)/include/imgui_vita2d

clean:
	@rm -rf $(TARGET).a $(TARGET).elf $(OBJS)
