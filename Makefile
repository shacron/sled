SDKDIR ?= ../sdk
BLD_BASEDIR ?= build
APPS ?= sled

BLD_HOST_OBJDIR ?= $(BLD_BASEDIR)/obj
BLD_HOST_BINDIR ?= $(BLD_BASEDIR)
BLD_HOST_LIBDIR ?= $(BLD_BASEDIR)/lib
BLD_HOST_INCDIR ?= $(SDKDIR)/include
BLD_HOST_OS ?= $(shell uname -s)
BLD_HOST_UNIVERSAL ?= 0
BLD_HOST_USE_SANITIZERS ?= 1

SILENT ?= @

##############################################################################
# C build environment
##############################################################################

# don't delete some intermediate files
.SECONDARY:

BLD_HOST_CC ?= clang
BLD_HOST_AS ?= clang
BLD_HOST_LD := clang
ifeq ($(BLD_HOST_OS),Darwin)
# libtool required for fat archives
BLD_HOST_AR := libtool
BLD_HOST_ARFLAGS := -static -o
else
BLD_HOST_AR ?= ar
BLD_HOST_ARFLAGS ?= -c -q
endif

LDFLAGS :=
CFLAGS  := -Wall -g -MMD
DEFINES :=
TOOLS :=

##############################################################################
# build type
##############################################################################

# Select by choosing BUILD on the command line
BUILD ?= release

ifeq ($(RV_TRACE),1)
DEFINES += -DRV_TRACE=1 -DWITH_SYMBOLS=1
endif

ifeq ($(BUILD),release)
CFLAGS += -O2
else
CFLAGS += -O0
DEFINES += -DBUILD_DEBUG=1
endif

ifeq ($(BLD_HOST_USE_SANITIZERS),1)
CFLAGS += -fsanitize=address,nullability,undefined -ftrivial-auto-var-init=pattern
endif

ifeq ($(BLD_HOST_UNIVERSAL),1)
CFLAGS += -arch arm64 -arch x86_64
endif

CXXFLAGS := $(CFLAGS) -std=c++17
LIBTARGETS :=
INCLUDES := -Iinclude
CSOURCES :=
CLIBFLAGS := -fvisibility=hidden -fPIC
DOBJS :=
PLATFORMS :=
DEVICES :=

##############################################################################
# include app defines
##############################################################################

define include_app

APP := $(1)
$(1)_LINK_LIBS :=
$(1)_CSOURCES :=
$(1)_DEFINES := $(DEFINES)
$(1)_INCLUDES := $(INCLUDES)

include app/$(1)/build.mk

LIBTARGETS += $$($(1)_LIBS)
INCLUDES += $$($(1)_INCLUDES)
$(1)_OBJS := $$($(1)_CSOURCES:%.c=$(BLD_HOST_OBJDIR)/%.c.o)
DOBJS += $$($(1)_OBJS)
PLATFORMS += $$($(1)_PLATFORM)

endef

$(foreach app,$(APPS),$(eval $(call include_app,$(app))))

##############################################################################
# include platform defines
##############################################################################

define include_plat

PLAT := $(1)
$(1)_INCLUDES :=
$(1)_DEVICES :=

include plat/$(1)/build.mk

DEVICES += $$($(1)_DEVICES)

endef

$(foreach plat,$(PLATFORMS),$(eval $(call include_plat,$(plat))))

##############################################################################
# top level build rules
##############################################################################

.PHONY: all
all: apps


$(BLD_HOST_OBJDIR)/core/%.c.o: core/%.c
	$(SILENT) mkdir -p $(dir $@)
	@echo " [cc]" $<
	$(SILENT) $(BLD_HOST_CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -Icore/inc -c -o $@ $<

$(BLD_HOST_OBJDIR)/dev/%.c.o: dev/%.c
	$(SILENT) mkdir -p $(dir $@)
	@echo " [cc]" $<
	$(SILENT) $(BLD_HOST_CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -Icore/inc -c -o $@ $<

##############################################################################
# device build rules
##############################################################################

include dev/build.mk

DEVICES := $(sort $(DEVICES))

define build_dev

$(1): $(BLD_HOST_LIBDIR)/dev/$(1).a

$(1)_OBJS := $$($(1)_CSOURCES:%.c=$(BLD_HOST_OBJDIR)/%.c.o)

$(BLD_HOST_LIBDIR)/dev/$(1).a: $$($(1)_OBJS)
	$(SILENT) mkdir -p $$(dir $$@)
	@echo " [ar]" $$(notdir $$@)
	$(SILENT) rm -f $$@
	$(SILENT) $(BLD_HOST_AR) $(BLD_HOST_ARFLAGS) $$@ $$^

endef

$(foreach dev,$(DEVICES),$(eval $(call build_dev,$(dev))))

devices: $(DEVICES:%=$(BLD_HOST_LIBDIR)/dev/%)

##############################################################################
# app build rules
##############################################################################

define build_app

$(1)_INCLUDES += $$($$($(1)_PLATFORM)_INCLUDES)
$(1)_DEVICE_LIBS := $$($$($(1)_PLATFORM)_DEVICES:%=$(BLD_HOST_LIBDIR)/dev/%.a)

$(1): $(BLD_HOST_BINDIR)/$(1)

$(BLD_HOST_BINDIR)/$(1): $($(1)_OBJS) $(BLD_HOST_LIBDIR)/libsled.a $$($(1)_DEVICE_LIBS)
	$(SILENT) mkdir -p $$(dir $$@)
	@echo " [ld]" $$(notdir $$@)
	$(SILENT) $(BLD_HOST_LD) $(CFLAGS) $(LDFLAGS) -o $$@ $$^

$(BLD_HOST_OBJDIR)/app/%.c.o: app/%.c
	$(SILENT) mkdir -p $$(dir $$@)
	@echo " [cc]" $$<
	$(SILENT) $(BLD_HOST_CC) $(CFLAGS) $$($(1)_INCLUDES) $($(1)_DEFINES) -c -o $$@ $$<

endef

$(foreach app,$(APPS),$(eval $(call build_app,$(app))))

apps: $(APPS:%=$(BLD_HOST_BINDIR)/%)

##############################################################################
# install rules
##############################################################################

$(BLD_HOST_INCDIR)/%.h: include/%.h
	$(SILENT) mkdir -p $(dir $@)
	$(SILENT) cp $^ $@

ifeq ($(MAKECMDGOALS),$(filter $(MAKECMDGOALS),install install_headers))
PUB_HEADERS := $(shell find include -name '*.h')
PUB_HEADERS := $(PUB_HEADERS:include/%=$(BLD_HOST_INCDIR)/%)
endif

install_headers: $(PUB_HEADERS)

install: apps install_headers


##############################################################################
# libsled build rules
##############################################################################

LIB_CSOURCES :=

include core/build.mk
include core/extension/build.mk

LIB_OBJS := $(LIB_CSOURCES:%.c=$(BLD_HOST_OBJDIR)/%.c.o)
DOBJS += $(LIB_OBJS)

lib: $(BLD_HOST_LIBDIR)/libsled.a

$(BLD_HOST_LIBDIR)/libsled.a: $(LIB_OBJS)
	@echo " [ar]" $(notdir $@)
	$(SILENT) mkdir -p $(dir $@)
	$(SILENT) rm -f $@
	$(SILENT) $(BLD_HOST_AR) $(BLD_HOST_ARFLAGS) $@ $^


##############################################################################
# others
##############################################################################

.PHONY: clean
clean:
	$(SILENT) rm -rf $(BLD_BASEDIR)


ifneq ($(MAKECMDGOALS),clean)
-include $(DOBJS:.o=.d)
endif