#
# Prevent inclusion of the Genode::Log definition after the vbox #define
# of 'Log'. Otherwise, the attempt to compile base/log.h will fail.
#
CC_OPT += -include base/log.h
