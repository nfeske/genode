TARGET = virtualbox6

#VBOX_CC_OPT += -DVBOX_WITH_HARDENING
#VBOX_CC_OPT += -DVBOX_WITH_GENERIC_SESSION_WATCHER

include $(REP_DIR)/lib/mk/virtualbox6-common.inc

CC_WARN += -Wall

SRC_CC := main.cc console.cc VirtualBoxErrorInfoImpl.cpp drivers.cc
SRC_CC += libc.cc unimpl.cc dummies.cc pdm.cc devices.cc mm.cc dynlib.cc

#         pgm.cc
#         hm.cc thread.cc

LIBS  += base
LIBS  += stdcxx
LIBS  += libiconv

CC_OPT_console = -DProgress=ClientProgress

LIB_MK_FILES := $(notdir $(wildcard $(REP_DIR)/lib/mk/virtualbox6-*.mk))

LIBS += $(LIB_MK_FILES:.mk=)

#LIBS  += qemu-usb

INC_DIR += $(call select_from_repositories,src/lib/libc)

INC_DIR += $(VBOX_DIR)/Runtime/include

#SRC_CC += HostServices/SharedFolders/service.cpp
#SRC_CC += HostServices/SharedFolders/mappings.cpp
#SRC_CC += HostServices/SharedFolders/vbsf.cpp
#SRC_CC += HostServices/SharedFolders/vbsfpath.cpp
#SRC_CC += HostServices/SharedFolders/vbsfpathabs.cpp
#SRC_CC += HostServices/SharedFolders/shflhandle.cpp
#
#SRC_CC += HostServices/SharedClipboard/service.cpp
#
#SRC_CC += HostServices/GuestProperties/service.cpp
SRC_CC += HostServices/common/message.cpp
#
#SRC_CC += frontend/dummy/virtualboxbase.cc
#SRC_CC += frontend/dummy/host.cc

INC_DIR += $(REP_DIR)/src/virtualbox6

INC_DIR += $(VBOX_DIR)/Main/include
INC_DIR += $(VBOX_DIR)/VMM/include

INC_DIR += $(VIRTUALBOX_DIR)/VBoxAPIWrap

INC_DIR += $(VBOX_DIR)/Main/xml
INC_DIR += $(VIRTUALBOX_DIR)/include/VBox/Graphics
INC_DIR += $(VBOX_DIR)/Main/src-server
INC_DIR += $(VBOX_DIR)/NetworkServices

# search path to 'scan_code_set_1.h'
INC_DIR += $(call select_from_repositories,src/drivers/ps2)

LIBS += blit

vpath %.cc $(REP_DIR)/src/virtualbox6/

CC_CXX_WARN_STRICT =
