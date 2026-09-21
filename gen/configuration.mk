# Copyright (c) TOLOSAT 2026
# SPDX-License-Identifier: Apache-2.0

# Configuration Makefile

ifndef BUILD_CONFIGURATION_MK
BUILD_CONFIGURATION_MK := yes

KCONF_SCRIPT = $(GEN_DIR)/Kconfig
DEFCONFIG_FILE = $(CONFIGS_DIR)/$(CONFIG_NAME)_defconfig

.PHONY : config menuconfig savedefconfig

config :
	$(KCONF) conf $(KCONF_SCRIPT)

menuconfig :
	$(KCONF) mconf $(KCONF_SCRIPT)

%_defconfig:
	@printf "$(BLUE)Loading $@ for $(LIB_NAME)...$(RESET)"
	@cp $(CONFIGS_DIR)/$@ $(CONFIG_FILE)
	@rm -f $(OLD_CONFIG_FILE)
	@rm -rf $(BUILD_DIR)
	@echo "$(BOLD)$(GREEN)Done.$(RESET)"

savedefconfig:
	@printf "$(BLUE)Saving $(CONFIG_NAME)_defconfig...$(RESET)"
	@cp $(CONFIG_FILE) $(DEFCONFIG_FILE)
	@echo "$(BOLD)$(GREEN)Done.$(RESET)"

endif # BUILD_CONFIGURATION_MK
