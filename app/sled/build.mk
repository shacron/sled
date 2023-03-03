APPPATH := app/$(APP)

$(APP)_PLATFORM := simple

$(APP)_INCLUDES += -I$(APPPATH)/inc -I$(BUILDDIR)/app/$(APP)

$(APP)_CSOURCES := \
	$(APPPATH)/cons.c \
	$(APPPATH)/main.c \

$(APP)_CXXSOURCES := \
