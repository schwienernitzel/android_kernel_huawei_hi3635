dtb-y += hi3635_asic_slt_config.dtb
dtb-y += hi3635_asic_config.dtb

targets += dtbs

dtbs: $(addprefix $(obj)/, $(dtb-y))
	$(Q)rm -f $(obj)/../*.dtb

clean-files := *.dtb
