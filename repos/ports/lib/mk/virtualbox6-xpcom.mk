include $(REP_DIR)/lib/mk/virtualbox6-common.inc

SRC_CC += nsCOMPtr.cpp nsComponentManagerUtils.cpp nsXPCOMGlue.cpp
SRC_C  += pratom.c

INC_DIR += $(VIRTUALBOX_DIR)/src/libs/xpcom18a4/nsprpub/pr/include/private
INC_DIR += $(VIRTUALBOX_DIR)/src/libs/xpcom18a4/xpcom/build

CC_OPT += -D_PR_PTHREADS

vpath nsCOMPtr.cpp                $(VIRTUALBOX_DIR)/src/libs/xpcom18a4/xpcom/glue
vpath nsComponentManagerUtils.cpp $(VIRTUALBOX_DIR)/src/libs/xpcom18a4/xpcom/glue
vpath pratom.c                    $(VIRTUALBOX_DIR)/src/libs/xpcom18a4/nsprpub/pr/src/misc
vpath nsXPCOMGlue.cpp             $(VIRTUALBOX_DIR)/src/libs/xpcom18a4/xpcom/glue/standalone

CC_OPT_nsXPCOMGlue := -DMOZ_DLL_SUFFIX= -Wno-literal-suffix

CC_CXX_WARN_STRICT =
