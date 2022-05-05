include $(REP_DIR)/lib/import/import-libc.mk

JEMALLOC_SRC_C := a.c b.c

vpath %.c $(REP_DIR)/src/lib/jemalloc

#
# We need to place all global variables of jemalloc in one consecutive
# range of the libc's data segment in order to reset them during 'execve'.
#
# To let the linker place all jemalloc variables consecutively, we assign
# their symbols to a dedicated section named '.data.jemalloc' using objcopy
# as a post-processing step.
#
# To make the libc aware of the start and end of the range, we generate
# two marker symbols '_jemalloc_start' and '_jemalloc_end', each in a
# distinct object file ('0_start.o' and 'z_end.o'). Genode's build system
# alphabetically sorts object files passed to the linker. Hence, the file
# prefixes ('0_' and 'z_') are choosen such that the start marker appears
# first and the end marker appears last.
#

UNFILTERED_OBJECTS := 0_start.o $(JEMALLOC_SRC_C:.c=.o) z_end.o

SRC_O := $(addprefix filtered-,$(UNFILTERED_OBJECTS))

# generate assembly code containing a data symbol with the given name
gen_symbol_s = echo ".data; .global _jemalloc_$1; _jemalloc_$1:"

0_start.s:
	$(call gen_symbol_s,start) > $@
z_end.s:
	$(call gen_symbol_s,end) > $@

FILTER_ARGS := --rename-section  .bss=.data.jemalloc \
               --rename-section .data=.data.jemalloc

filtered-%.o: %.o
	$(OBJCOPY) $(FILTER_ARGS) $< $@

# keep intermediate objects to avoid 'rm ...' messages printed by make
.PRECIOUS: $(UNFILTERED_OBJECTS)
