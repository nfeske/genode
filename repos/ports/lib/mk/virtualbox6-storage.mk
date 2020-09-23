include $(REP_DIR)/lib/mk/virtualbox6-common.inc

SRC_CC += $(addprefix Storage/, $(notdir $(wildcard $(VBOX_DIR)/Storage/*.cpp)))

CC_CXX_WARN_STRICT =
