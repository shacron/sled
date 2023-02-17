SDKDIR ?= ../sdk
BLD_BASEDIR ?= build
APPS ?= sled
APP_PLATFORM ?= simple

BLD_HOST_OBJDIR ?= $(BLD_BASEDIR)/obj
BLD_HOST_BINDIR ?= $(BLD_BASEDIR)
BLD_HOST_LIBDIR ?= $(BLD_BASEDIR)/lib
BLD_HOST_INCDIR ?= $(SDKDIR)/include


##############################################################################
# C build environment
##############################################################################

# don't delete some intermediate files
.SECONDARY:

BLD_HOST_CC ?= clang
BLD_HOST_AS ?= clang
BLD_HOST_LD := clang
BLD_HOST_AR ?= ar

LDFLAGS :=
CFLAGS  := -Wall -g -MMD -fsanitize=address,nullability,undefined -ftrivial-auto-var-init=pattern
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
	@$(BLD_HOST_AR) -c -q $@ $^


##############################################################################
# others
##############################################################################

.PHONY: clean
clean:
	@rm -rf $(BLD_BASEDIR)


ifneq ($(MAKECMDGOALS),clean)
-include $(DOBJS:.o=.d)
endif