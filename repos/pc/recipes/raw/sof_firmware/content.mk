PORT_DIR := $(call port_dir,$(REP_DIR)/ports/sof)

content: ucode_files LICENSE.Intel

.PHONY: ucode_files

ucode_files:
	find -L $(PORT_DIR) -type f -name *.tplg -exec cp {} . ";"
	find -L $(PORT_DIR) -type f -name *.ri -exec cp {} . ";"

LICENSE.Intel:
	cp $(PORT_DIR)/firmware/LICENCE.Intel $@

