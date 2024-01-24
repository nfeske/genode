SRC_CC = vfs.cc

# reuse glyphs_file_system.h from vfs_ttf
INC_DIR += $(REP_DIR)/src/lib/vfs/ttf

LIBS  += bdf_font

vpath %.cc $(REP_DIR)/src/lib/vfs/bdf

SHARED_LIB = yes
