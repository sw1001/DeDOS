OPTIONS+=-DPICO_SUPPORT_ICMP4
MOD_OBJ+=$(LIBBASE)/pico_icmp4.o
ifneq ($(PING),0)
  OPTIONS+=-DPICO_SUPPORT_PING
endif
