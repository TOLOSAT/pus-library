# Copyright (c) TOLOSAT 2026
# SPDX-License-Identifier: Apache-2.0

# Makefile with library paths

ifndef PATH_MK
PATH_MK := yes

##############################################
############# LIBRARY DIRECTORIES ############
##############################################

# Depending if the workspace is provided by the top level or not
ifeq ($(origin WORKSPACE), undefined)
LIB_DIR 	= $(CURDIR)
BUILD_DIR 	= $(LIB_DIR)/build
else
LIB_DIR  	= $(WORKSPACE)/middlewares/$(LIB_NAME)-library
BUILD_DIR   = $(WORKSPACE)/build
endif

GEN_DIR         = $(LIB_DIR)/gen
CONFIGS_DIR     = $(LIB_DIR)/configs
TOOLS_DIR      ?= $(LIB_DIR)/tools
BUILD_STATE_DIR = $(BUILD_DIR)/state
OUTPUT_DIR      = $(BUILD_DIR)/libs
OBJDIR          = $(BUILD_DIR)/middlewares/$(LIB_NAME)

endif # PATH_MK #
