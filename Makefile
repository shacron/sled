SDKDIR ?= ../sdk
BLD_BASEDIR ?= build
APPS ?= sled
APP_PLATFORM ?= simple

BLD_HOST_OBJDIR ?= $(BLD_BASEDIR)/obj
BLD_HOST_BINDIR ?= $(BLD_BASEDIR)
BLD_HOST_LIBDIR ?= $(BLD_BASEDIR)/lib
BLD_HOST_INCDIR ?= $(SDKDIR)/include
BLD_HOST_OS ?= $(shell uname -s)
BLD_HOST_UNIVERSAL ?= 0
BLD_HOST_USE_SANITIZERS ?= 1

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
INCLUDES := -Iinclude -Iplat/$(APP_PLATFORM)/inc
CSOURCES :=
CLIBFLAGS := -fvisibility=hidden -fPIC
DOBJS :=

##############################################################################
# include app defines
##############################################################################

define include_app

APP := $(1)
$(1)_LINK_LIBS :=
$(1)_CSOURCES :=
$(1)_INCLUDES :=

include app/$(1)/build.mk

LIBTARGETS += $$($(1)_LIBS)
INCLUDES += $$($(1)_INCLUDES)
$(1)_OBJS := $$($(1)_CSOURCES:%.c=$(BLD_HOST_OBJDIR)/%.c.o)
DOBJS += $$($(1)_OBJS)

endef

$(foreach app,$(APPS),$(eval $(call include_app,$(app))))


##############################################################################
# top level build rules
##############################################################################

$(BLD_HOST_OBJDIR)/core/%.c.o: core/%.c
	@mkdir -p $(dir $@)
	@echo " [cc]" $<
	@$(BLD_HOST_CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -Icore/inc -c -o $@ $<

$(BLD_HOST_OBJDIR)/dev/%.c.o: dev/%.c
	@mkdir -p $(dir $@)
	@echo " [cc]" $<
	@$(BLD_HOST_CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -Icore/inc -c -o $@ $<

$(BLD_HOST_OBJDIR)/app/%.c.o: app/%.c
	@mkdir -p $(dir $@)
	@echo " [cc]" $<
	@$(BLD_HOST_CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -o $@ $<


##############################################################################
# app build rules
##############################################################################

define build_app

$(1): $(BLD_HOST_BINDIR)/$(1)

$(BLD_HOST_BINDIR)/$(1): $($(1)_OBJS) $(BLD_HOST_LIBDIR)/libsled.a
	@mkdir -p $$(dir $$@)
	@echo " [ld]" $$(notdir $$@)
	@$(BLD_HOST_LD) $(CFLAGS) $(LDFLAGS) -o $$@ $$^

endef

$(foreach app,$(APPS),$(eval $(call build_app,$(app))))

apps: $(APPS:%=$(BLD_HOST_BINDIR)/%)


##############################################################################
# install rules
##############################################################################

$(BLD_HOST_INCDIR)/%.h: include/%.h
	@mkdir -p $(dir $@)
	@cp $^ $@

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

include dev/build.mk
include core/build.mk
include core/extension/build.mk

LIB_OBJS := $(LIB_CSOURCES:%.c=$(BLD_HOST_OBJDIR)/%.c.o)
DOBJS += $(LIB_OBJS)

lib: $(BLD_HOST_LIBDIR)/libsled.a

$(BLD_HOST_LIBDIR)/libsled.a: $(LIB_OBJS)
	@echo " [ar]" $(notdir $@)
	@mkdir -p $(dir $@)
	@rm -f $@
	@$(BLD_HOST_AR) $(BLD_HOST_ARFLAGS) $@ $^


##############################################################################
# others
##############################################################################

.PHONY: clean
clean:
	@rm -rf $(BLD_BASEDIR)


ifneq ($(MAKECMDGOALS),clean)
-include $(DOBJS:.o=.d)
endif