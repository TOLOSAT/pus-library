# Makefile including all build recipes

ifndef BUILD_MK
BUILD_MK := yes

##############################################
############# DIRECTORIES & FILES ############
##############################################

# Directories
PUBLIC_INCDIR = $(LIB_DIR)/include
COMPONENTS_DIR = $(LIB_DIR)/components
OBJDIR = $(BUILD_DIR)/middlewares/$(LIB_NAME)

# Files
COMPONENTS = $(notdir $(wildcard $(COMPONENTS_DIR)/*))
SRCS = $(foreach component,$(COMPONENTS),$(wildcard $(COMPONENTS_DIR)/$(component)/src/*.c))
OBJS = $(foreach component,$(COMPONENTS), \
	   $(patsubst $(COMPONENTS_DIR)/$(component)/src/%.c,$(OBJDIR)/components/$(component)/%.o, \
	   $(filter $(COMPONENTS_DIR)/$(component)/src/%.c,$(SRCS))))
LIB  = $(OUTPUT_DIR)/lib$(LIB_NAME).a

##############################################
#################### FLAGS ###################
##############################################

INCDIRS			 = $(PUBLIC_INCDIR) $(COMPONENTS_DIR) $(KERNEL_HEADERS) $(EXTRA_INCS)
PRIVATE_INCDIRS	 = $(foreach component,$(COMPONENTS),$(wildcard $(COMPONENTS_DIR)/$(component)/inc))
INCFLAGS		 = $(addprefix -I,$(INCDIRS))
CHECKER_INCFLAGS = $(INCFLAGS) $(addprefix -I,$(PRIVATE_INCDIRS))

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
	@$(foreach dir,$(patsubst $(WORKSPACE)/%,%,$(PRIVATE_INCDIRS)),echo "  - $(dir) (private)";)
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
define COMPONENT_BUILD_RULE
$(OBJDIR)/components/$(1)/%.o : $(COMPONENTS_DIR)/$(1)/src/%.c
	@echo "  CC  [$(1)] $$(@F)"
	@mkdir -p $$(@D)
	@$(CC) $(CFLAGS) $(INCFLAGS) $(if $(wildcard $(COMPONENTS_DIR)/$(1)/inc),-iquote $(COMPONENTS_DIR)/$(1)/inc) $$< -o $$@
endef

$(foreach component,$(COMPONENTS),$(eval $(call COMPONENT_BUILD_RULE,$(component))))

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
