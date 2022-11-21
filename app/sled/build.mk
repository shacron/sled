APPPATH := app/$(APP)

$(APP)_INCLUDES += -I$(APPPATH)/inc -I$(BUILDDIR)/app/$(APP)

$(APP)_CSOURCES := \
	$(APPPATH)/main.c \

$(APP)_CXXSOURCES := \
