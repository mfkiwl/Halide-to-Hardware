# Usage:
#  make all:                    compiles all code without running
#       generator:              create Halide generator
#       design:                 create cpu design
#       design-clockwork:       create clockwork design
#       image:                  create an image with random data
#       run:                    run cpu design with image
#       run-clockwork:          run clockwork design with image
#       compare:                compare two output images
#       test:                   run and compare to cpu output
#       eval:                   evaluate runtime
#       clean:                  remove bin directory

###############################################################################
# MODIFICATION: NONE
#------------------------------------------------------------------------------
# Makefile parameter/variable declarations
#
###############################################################################

include ../../hw_support/Makefile.inc



###############################################################################
# MODIFICATION: OPTIONAL
#------------------------------------------------------------------------------
# App-specific info
#
# TESTNAME                      : name of the app
# USE_CORE_IR_VALID             : whether to generate valid signal for coreir codegen
# HL_TARGET                     : Halide target
# RDAI_PLATFORM_RUNTIME         : RDAI platform runtime to use 
###############################################################################

TESTNAME                        = audio
USE_COREIR_VALID                = 1
HL_TARGET                       = host-x86-64
RDAI_PLATFORM_RUNTIME           = clockwork_sim



###############################################################################
# MODIFICATION : NONE
#------------------------------------------------------------------------------
# Include hardwrae build targets
#
###############################################################################

include ../../hw_support/hardware_targets.mk







# g++ audio_generator.cpp /mnt/c/users/kosho/desktop/aha/Halide-to-Hardware/tools/GenGen.cpp -g -std=c++17 -fno-rtti -I /mnt/c/users/kosho/desktop/aha/Halide-to-Hardware/include -L /mnt/c/users/kosho/desktop/aha/Halide-to-Hardware/bin -lHalide -lpthread -ldl -o a.out

