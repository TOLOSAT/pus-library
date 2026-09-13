# PUS library Makefile

##############################################
################ CONFIGURATION ###############
##############################################

# Software Version
MAJOR = 0
MINOR = 3
PATCH = 5

##############################################
################### MAKE #####################
##############################################

LIB_NAME = pus

# Main recipe
all : build

##############################################
################## INCLUDES ##################
##############################################

include gen/paths.mk
include gen/settings.mk
include gen/build.mk
include gen/verification.mk

##############################################
################## COLORS ####################
##############################################

# Colors for terminal output
GREEN = \033[92m
YELLOW = \033[93m
RED = \033[91m
BLUE = \033[94m

# Fonts settings
BOLD = \033[1m
NORMAL = \033[0m
ITALIC = \033[3m
UNDERLINE = \033[4m

# Reset effect
RESET = \033[0m