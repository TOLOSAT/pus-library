# Makefile including all build recipes

ifndef BUILD_MK
BUILD_MK := yes

##############################################
############# DIRECTORIES & FILES ############
##############################################

# Directories
INCDIR = $(LIB_DIR)/inc
SRCDIR = $(LIB_DIR)/src
OBJDIR = $(BUILD_DIR)/middlewares/$(LIB_NAME)

# Files
SRCS = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*/*.c)
OBJS = $(subst $(SRCDIR)/,$(OBJDIR)/,$(SRCS:.c=.o))
LIB  = $(OUTPUT_DIR)/lib$(LIB_NAME).a

##############################################
#################### FLAGS ###################
##############################################

INCDIRS		= $(INCDIR) $(KERNEL_HEADERS) $(EXTRA_INCS)
INCFLAGS	= $(addprefix -I,$(INCDIRS))

##############################################
################ BUILD RECIPES ###############
##############################################

.PHONY : build start end clean
build: end
end: $(LIB)
$(OBJS): | start

define LIBRARY_START_VERBOSE
	@echo "$(BOLD)=============================$(RESET)"
	@echo "$(BOLD)===        LIBRARY        ===$(RESET)"
	@echo "$(BOLD)=============================$(RESET)"
	@echo "$(YELLOW)Library name:$(RESET) $(LIB_NAME)"
	@echo "$(YELLOW)Files to compile:$(RESET) $(words $(SRCS))"
	@echo "$(YELLOW)Compilation Flags:$(RESET)"
	@echo $(CFLAGS)
	@echo "$(YELLOW)Include Paths:$(RESET)"
	@$(foreach dir,$(patsubst $(WORKSPACE)/%,%,$(INCDIRS)),echo "  - $(dir)";)
	@echo "$(BLUE)Start building...$(RESET)"
endef

define LIBRARY_END_VERBOSE
	@echo "$(BOLD)$(GREEN)Done.$(RESET)"
	@echo ""
endef

# Include dependencies
-include $(OBJS:.o=.d)

# Build header
start :
	$(if $(PARALLEL_BUILD),$(QUIET_RECIPE),$(LIBRARY_START_VERBOSE))

# Building recipes
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@echo "  CC  $(@F)"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(INCFLAGS) $< -o $@

# Library generation
$(LIB) : $(OBJS)
	@echo "  AR  $(@F)"
	@mkdir -p $(@D)
	@$(AR) rcs $@ $^

# Build footer
end :
	$(if $(PARALLEL_BUILD),$(QUIET_RECIPE),$(LIBRARY_END_VERBOSE))

# Clean recipe
clean :
	@printf "$(BLUE)Cleaning $(LIB_NAME) build directory...$(RESET)"
	@rm -rf $(OBJDIR)
	@rm -rf $(LIB)
	@echo "$(BOLD)$(GREEN)Done.$(RESET)"

endif # BUILD_MK #
