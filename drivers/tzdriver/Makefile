obj-$(CONFIG_TZDRIVER) += tc_client_driver.o teek_client_api.o
obj-$(CONFIG_TZDRIVER) += smc.o agent.o gp_ops.o mem.o
obj-$(CONFIG_TEELOG) += tlogger.o
ifeq ($(TARGET_SUPPORT_PLAYREADY), true) 
ccflags-y += -DTARGET_SUPPORT_PLAYREADY
endif 
