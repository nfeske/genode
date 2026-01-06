TARGET   = fb_sdl
LIBS     = lx_hybrid blit
REQUIRES = linux
SRC_CC   = main.cc
SDL_INC  = /usr/include/SDL2
SDL_LIB  = sdl2

#
# SDL_3 support
#
# CC_OPT += -DSDL3
# SDL_INC = /usr/include/SDL3
# SDL_LIB = sdl3
#

LX_LIBS  = $(SDL_LIB)
INC_DIR += $(PRG_DIR) $(SDL_INC)

# accept conversion warnings caused by libgcc's include/arm_neon.h
CC_OPT  += -Wno-error=narrowing \
           -Wno-error=float-conversion \
           -Wno-error=unused-parameter \
           -Wno-error=cpp

#
# Fix builtin declaration mismatch error for 'imaxabs' in
# /usr/include/inttypes.h
#
CC_OPT += -Wno-builtin-declaration-mismatch
