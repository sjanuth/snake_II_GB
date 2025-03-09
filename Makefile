#
# Simple Makefile that compiles all .c and .s files in the same folder
#

# If you move this project you can change the directory 
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
# IMPORTANT: Use absolute paths here, otherwise bear will fail to genereate
# compile_commands.json required for clangd to work properly
ifndef GBDK_HOME
	GBDK_HOME = /home/sja/Programming/gbdk/
endif

LCC = $(GBDK_HOME)bin/lcc 

# GBDK_DEBUG = ON
ifdef GBDK_DEBUG
	LCCFLAGS += -debug -v
endif

# Bank configuraition
# ya4 = 4 RAM banks
# yo4 = 4 ROM banks
# yt0x1a =  1A-ROM+MBC5+RAM Cartridge type

LCCFLAGS += -Wl-yt0x1A -Wl-yo4 -Wl-ya4 -autobank

# You can set the name of the .gb ROM file here
PROJECTNAME    = Snake_3310_GB

BINS	    = $(PROJECTNAME).gb
SPRITESDIR  = sprites
SPRITE_SOURCES := $(wildcard $(SPRITESDIR)/*.c)
# Convert .c file paths into .o file paths (keeping them in sprites/)
#SPRITE_OBJ := $(SPRITE_SOURCES:.c=.o)
C_ROOT_SOURCES   := $(wildcard *.c)

FONTSDIR = vwf/src
FONTSINCLUDE = vwf/include
FONTS_SOURCES := $(wildcard $(FONTSDIR)/*.c)
CSOURCES = $(C_ROOT_SOURCES) $(SPRITE_SOURCES) $(FONTS_SOURCES)
ASMSOURCES := $(wildcard *.s)
ASM_FONTS_SOURCES := $(wildcard $(FONTSDIR)/sm83/*.s)

LCCFLAGS += -I$(FONTSINCLUDE) 

all:	$(BINS) 

compile.bat: Makefile
	@echo "REM Automatically generated from Makefile" > compile.bat
	@make -sn | sed y/\\//\\\\/ | sed s/mkdir\ \-p/mkdir/ | grep -v make >> compile.bat


# Compile and link all source files in a single call to LCC
#
$(BINS):	$(CSOURCES) $(ASMSOURCES) $(ASM_FONTS_SOURCES)
	$(LCC) $(LCCFLAGS) -o $@ $(CSOURCES) $(ASMSOURCES) $(ASM_FONTS_SOURCES)
	
# Compiles sprite assets in sprites directory
$(SPRITESDIR)/%.o:	$(SPRITESDIR)/%.c
	@echo "compile sprites"
	$(LCC) $(LCCFLAGS) -o $@ $<

clean:
	rm -f *.o *.lst *.map *.gb *.ihx *.sym *.cdb *.adb *.asm *.noi *.rst $(SRC_DIR)/*.o

