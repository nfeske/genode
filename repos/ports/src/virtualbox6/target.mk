TARGET = virtualbox6

include $(REP_DIR)/src/virtualbox6/target.inc

LIBS += virtualbox6
LIBS += blit

vpath frontend/% $(REP_DIR)/src/virtualbox6/
vpath %.cc       $(REP_DIR)/src/virtualbox6/

CC_CXX_WARN_STRICT =
