TITLE       := Freely Dev
VERSION     := 1.00
TITLE_ID    := FREY00001
CONTENT_ID  := IV0000-FREY00001_00-FREELYDEV0000001

TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJDIR     := $(shell basename $(CURDIR))
INTDIR      := $(PROJDIR)/x64/Debug
SOURCE      := $(PROJDIR)/main.cpp
OBJECT      := $(INTDIR)/main.o
COMMONDIR   := $(PROJDIR)
COMMON_OBJ  := $(INTDIR)/graphics.o
PKGTOOL     := $(TOOLCHAIN)/bin/linux/PkgTool.Core
FSELF       := $(TOOLCHAIN)/bin/linux/create-fself
GP4TOOL     := $(TOOLCHAIN)/bin/linux/create-gp4

CCX         := clang++
LD          := ld.lld

CXXFLAGS    := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/c++/v1 -I$(COMMONDIR)
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib -lc -lkernel -lc++ -lSceVideoOut $(TOOLCHAIN)/lib/crt1.o

.PHONY: all clean
all: $(CONTENT_ID).pkg

$(INTDIR):
	mkdir -p $(INTDIR)

$(OBJECT): $(SOURCE) $(COMMONDIR)/graphics.h | $(INTDIR)
	$(CCX) $(CXXFLAGS) -o $@ $<

$(COMMON_OBJ): $(COMMONDIR)/graphics.cpp $(COMMONDIR)/graphics.h | $(INTDIR)
	$(CCX) $(CXXFLAGS) -o $@ $<

eboot.bin: $(OBJECT) $(COMMON_OBJ)
	$(LD) $(OBJECT) $(COMMON_OBJ) -o $(INTDIR)/freely.elf $(LDFLAGS)
	$(FSELF) --in=$(INTDIR)/freely.elf --out=$(INTDIR)/freely.oelf --eboot "$@" --paid 0x3800000000000011
	test -s "$@"

sce_sys/param.sfo:
	mkdir -p sce_sys
	$(PKGTOOL) sfo_new $@
	$(PKGTOOL) sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOL) sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOL) sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOL) sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOL) sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOL) sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOL) sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

sce_sys/icon0.png: tools/make_icon.py
	python3 tools/make_icon.py $@

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png
	$(GP4TOOL) -out=$@ --content-id=$(CONTENT_ID) --files "$^"

$(CONTENT_ID).pkg: pkg.gp4
	$(PKGTOOL) pkg_build $< .
	$(PKGTOOL) pkg_validate --verbose $@
	$(PKGTOOL) pkg_listentries $@

clean:
	rm -rf $(INTDIR) eboot.bin pkg.gp4 $(CONTENT_ID).pkg sce_sys/param.sfo sce_sys/icon0.png
