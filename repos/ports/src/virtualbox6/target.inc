#VBOX_CC_OPT += -DVBOX_WITH_HARDENING
#VBOX_CC_OPT += -DVBOX_WITH_GENERIC_SESSION_WATCHER

include $(REP_DIR)/lib/mk/virtualbox6-common.inc

CC_WARN += -Wall

SRC_CC = frontend/main.cc frontend/console.cc \
         frontend/VirtualBoxErrorInfoImpl.cpp

SRC_CC += sup_vmm.cc libc.cc rt.cc unimpl.cc dummies.cc

#         devices.cc drivers.cc \
#         mm.cc pdm.cc pgm.cc
#         hm.cc thread.cc dynlib.cc

LIBS  += base
LIBS  += stdcxx

LIBS  += virtualbox6-bios virtualbox6-runtime \
         virtualbox6-vmm virtualbox6-devices \
         virtualbox6-storage virtualbox6-zlib virtualbox6-liblzf \
         virtualbox6-xml virtualbox6-main virtualbox6-apiwrap \
         virtualbox6-dis

LIBS  += libiconv

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
#
SRC_CC += frontend/dummy/errorinfo.cc
#SRC_CC += frontend/dummy/virtualboxbase.cc
#SRC_CC += frontend/dummy/autostart.cc frontend/dummy/rest.cc
#SRC_CC += frontend/dummy/host.cc

INC_DIR += $(REP_DIR)/src/virtualbox6

INC_DIR += $(VBOX_DIR)/Main/include
INC_DIR += $(VBOX_DIR)/VMM/include

INC_DIR += $(REP_DIR)/src/virtualbox6/frontend
INC_DIR += $(VIRTUALBOX_DIR)/VBoxAPIWrap

INC_DIR += $(VBOX_DIR)/Main/xml
INC_DIR += $(VIRTUALBOX_DIR)/include/VBox/Graphics
#INC_DIR += $(VBOX_DIR)/HostServices
#
# search path to 'scan_code_set_2.h'
INC_DIR += $(call select_from_repositories,src/drivers/ps2)
