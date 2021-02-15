#
# Selection of ARCH-specific subdirectories and files
#

LIB_MK_SPECS(arm_v6)  := arm arm_v6
LIB_MK_SPECS(arm_v7a) := arm arm_v7 cortex_a8 cortex_a9 cortex_a15
LIB_MK_SPECS(arm_v8a) := arm_64 arm_v8
LIB_MK_SPECS(x64_64)  := x86_64
LIB_MK_SPECS(riscv)   := riscv

SRC_BOOTSTRAP_SPECS(arm_v6)  := arm
SRC_BOOTSTRAP_SPECS(arm_v7a) := arm
SRC_BOOTSTRAP_SPECS(arm_v8a) := arm_64 arm
SRC_BOOTSTRAP_SPECS(x86_64)  := x86_64
SRC_BOOTSTRAP_SPECS(riscv)   := riscv

SRC_CORE_SPECS(arm_v6)  := arm_v6
SRC_CORE_SPECS(arm_v7a) := arm cortex_a8 cortex_a9 cortex_a15
SRC_CORE_SPECS(arm_v8a) := arm arm_v8
SRC_CORE_SPECS(x86_64)  := x86_64
SRC_CORE_SPECS(riscv)   := riscv

SRC_INCLUDE_HW_SPECS(arm_v6)  := arm
SRC_INCLUDE_HW_SPECS(arm_v7a) := arm
SRC_INCLUDE_HW_SPECS(arm_v8a) := arm_64 arm
SRC_INCLUDE_HW_SPECS(x86_64)  := x86_64
SRC_INCLUDE_HW_SPECS(riscv)   := riscv

SRC_LIB_BASE_SPECS(arm_v6)  := arm
SRC_LIB_BASE_SPECS(arm_v7a) := arm
SRC_LIB_BASE_SPECS(arm_v8a) := arm_64
SRC_LIB_BASE_SPECS(x64_64)  := x86_64
SRC_LIB_BASE_SPECS(riscv)   := riscv

SRC_LIB_STARTUP_SPECS(arm_v6)  := arm
SRC_LIB_STARTUP_SPECS(arm_v7a) := arm
SRC_LIB_STARTUP_SPECS(arm_v8a) := arm_64
SRC_LIB_STARTUP_SPECS(x64_64)  := x86_64
SRC_LIB_STARTUP_SPECS(riscv)   := riscv

SRC_CORE_SPECS(x64_64) := x86

# location of hw-ld.mk file
LD_MK_DIR(arm_v6)  := lib/mk/spec/arm
LD_MK_DIR(arm_v7a) := lib/mk/spec/arm
LD_MK_DIR(arm_v8a) := lib/mk/spec/arm_v8
LD_MK_DIR(x64_64)  := lib/mk/spec/x86_64
LD_MK_DIR(riscv)   := lib/mk/spec/riscv


#
# Enumeration of package content
#

BASE_DIR    := $(GENODE_DIR)/repos/base
BASE_HW_DIR := $(GENODE_DIR)/repos/base-hw


SRC_TIMER := target.inc include hw main.cc

SRC_INCLUDE_HW += $(notdir $(wildcard $(BASE_HW_DIR)/src/include/hw/*.h)) \
                  $(addprefix spec/,${SRC_INCLUDE_HW_SPECS(${ARCH})})

SRC_BOOTSTRAP += hw env.cc init.cc lock.cc log.cc thread.cc \
                 platform.cc platform.h \
                 $(addprefix spec/,${SRC_BOOTSTRAP_SPECS(${ARCH})}) \
                 $(addprefix board/,$(BOARD))

SRC_LIB_BASE += $(notdir $(wildcard $(BASE_HW_DIR)/src/lib/base/*.cc)) \
                $(notdir $(wildcard $(BASE_DIR)/src/lib/base/*.cc)) \
                ${SRC_LIB_BASE_SPECS(${ARCH})}

SRC_LIB_STARTUP += init_main_thread.cc _main.cc \
                   ${SRC_LIB_STARTUP_SPECS(${ARCH})}

SRC_CORE += $(notdir $(wildcard $(BASE_HW_DIR)/src/core/*.cc)) \
            $(notdir $(wildcard $(BASE_HW_DIR)/src/core/*.h)) \
            $(notdir $(wildcard $(BASE_DIR)/src/core/*.cc)) \
            $(addprefix spec/,${SRC_CORE_SPECS(${ARCH})}) \
            $(addprefix board/,$(BOARD)) \
            version.inc target.inc include hw kernel

LIB_MK := base-hw-common.mk base-hw.mk bootstrap-hw.inc core-hw.inc \
          timeout-hw.mk cxx.mk base.inc base-common.inc startup.inc \
          $(addprefix spec/,${LIB_MK_SPECS(${ARCH})})

CONTENT := $(addprefix src/timer/,$(SRC_TIMER)) \
           $(addprefix src/include/hw/,$(SRC_INCLUDE_HW)) \
           $(addprefix src/bootstrap/,$(SRC_BOOTSTRAP)) \
           $(addprefix lib/mk/,$(LIB_MK)) \
           $(addprefix src/lib/base/,$(SRC_LIB_BASE)) \
           $(addprefix src/lib/base/,$(SRC_LIB_STARTUP)) \
           $(addprefix src/core/,$(SRC_CORE)) \
           src/lib/hw src/lib/ld src/lib/cxx \
           src/include/base/internal src/include/pager \
           include/drivers/uart

# remove duplicates
CONTENT := $(sort $(CONTENT))


#
# Utilities to aggregate content from base, base-hw, and rep directories
#

_try_mirror_from = $(if $(wildcard $1/$2),mkdir -p $(dir $2); cp -r $1/$2 $(dir $2))

mirror_from_base_dir    = $(call _try_mirror_from,$(GENODE_DIR)/repos/base,$@)
mirror_from_base_hw_dir = $(call _try_mirror_from,$(GENODE_DIR)/repos/base-hw,$@)
mirror_from_rep_dir     = $(call _try_mirror_from,$(REP_DIR),$@)


#
# Rules
#

content: $(CONTENT)

$(CONTENT):
	$(mirror_from_base_dir)
	$(mirror_from_base_hw_dir)
	$(mirror_from_rep_dir)

content: LICENSE
LICENSE:
	cp $(GENODE_DIR)/LICENSE $@

content: etc/specs.conf
etc/specs.conf:
	$(mirror_from_base_hw_dir)

content: generalize_target_names
generalize_target_names: $(CONTENT)
	mv ${LD_MK_DIR(${ARCH})}/ld-hw.mk ${LD_MK_DIR(${ARCH})}/ld.mk
	sed -i "s/ld-hw/ld/"           src/lib/ld/hw/target.mk
	sed -i "s/hw_timer_drv/timer/" src/timer/hw/target.mk
	sed -i "s/\?= unknown/:= $(BOARD)/" src/core/hw/target.mk
	sed -i "s/\?= unknown/:= $(BOARD)/" src/bootstrap/hw/target.mk

