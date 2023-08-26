PLATPATH := plat/$(PLAT)

$(PLAT)_INCLUDES := -I$(PLATPATH)/inc

$(PLAT)_DEVICES := \
	sled_uart \
	sled_rtc \
	sled_intc \
	sled_mpu \
	sled_timer \

