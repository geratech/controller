# ----------------------------------------------------------------------------
# This file is part of the Synthetos g2core project


# To compile:
#   make BOARD=gShield
# Or:
#   make BOARD=shopbotShield

# You can also choose a CONFIG from boards.mk:
#   make CONFIG=ShapeokoDualY BOARD=gShield



# Backward compatibility with old projects that use PLATFORM instead, but with a warning:
ifneq ("$(PLATFORM:-)","")
    $(warning Using PLATFORM value of $(PLATFORM) as BOARD.)
    $(warning Please swich to using BOARD on the command line.)
    BOARD = $(PLATFORM)
endif

##########
# BOARDs for use directly from the make command line (with default settings) or by CONFIGs.

ifeq ("$(BOARD)","geratech_proto")
    # This is a due with a Synthetos gShield. We'll use the Due platform, but set defines
    # for the code to get the pinout right.

    # Note: we call it "g2core-due" instead of "due" since the Motate built-in provides
    # a "due" BASE_BOARD.
    BASE_BOARD = geratech-due
    DEVICE_DEFINES += MOTATE_BOARD="geratech_proto"
    DEVICE_DEFINES += SETTINGS_FILE=${SETTINGS_FILE}
endif

##########

ifeq ("$(BASE_BOARD)","geratech-due")
    _BOARD_FOUND = 1

    DEVICE_DEFINES += MOTATE_CONFIG_HAS_USBSERIAL=1

    FIRST_LINK_SOURCES += $(sort $(wildcard ${MOTATE_PATH}/Atmel_sam_common/*.cpp)) $(sort $(wildcard ${MOTATE_PATH}/Atmel_sam3x/*.cpp))

    CHIP = SAM3X8E
    export CHIP
    CHIP_LOWERCASE = sam3x8e

    BOARD_PATH = ./board/geratech_proto
		SOURCE_DIRS += ${BOARD_PATH} device/step_dir_driver
		#SOURCE_DIRS += ${BOARD_PATH} device/step_dir_geratech_servo

		#SOURCE_DIRS += ./board/geratech_proto/libsam/source
		#DEVICE_INCLUDE_DIRS += ./board/geratech_proto/libsam/include
		#DEVICE_INCLUDE_DIRS += ./board/geratech_proto/libsam/

    PLATFORM_BASE = ${MOTATE_PATH}/platform/atmel_sam

    include $(PLATFORM_BASE).mk
endif
