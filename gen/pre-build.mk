# Copyright (c) TOLOSAT 2026
# SPDX-License-Identifier: Apache-2.0

# Makefile including scoped autoconf generation

ifndef AUTOCONF_MK
AUTOCONF_MK := yes

AUTOCONF_DIR = $(OBJDIR)/conf
AUTOCONF_SRC = $(AUTOCONF_DIR)/$(LIB_NAME)_autoconf.h
AUTOCONF_STAMP = $(AUTOCONF_DIR)/autoconf.stamp
AUTOCONF_GENERATOR = $(TOOLS_DIR)/config-parser.py
AUTOCONF_KCONFIGS = $(GEN_DIR)/Kconfig.options

ifeq ($(wildcard $(AUTOCONF_SRC)),)
.PHONY : autoconf-missing
$(AUTOCONF_STAMP) : autoconf-missing
endif

.PHONY : pre-build autoconf pre-build-clean

pre-build : autoconf
autoconf : $(AUTOCONF_SRC)

$(AUTOCONF_SRC) : | $(AUTOCONF_STAMP)

$(AUTOCONF_STAMP) : $(CONFIG_FILE) $(AUTOCONF_GENERATOR) $(AUTOCONF_KCONFIGS)
	@echo "  PY  [$(LIB_NAME)/generated] $(notdir $(AUTOCONF_SRC))"
	@mkdir -p $(@D)
	@$(PYTHON) $(AUTOCONF_GENERATOR) -i $(CONFIG_FILE) -o $(@D) \
		--header-name $(notdir $(AUTOCONF_SRC)) \
		$(foreach kconfig,$(AUTOCONF_KCONFIGS),--kconfig $(kconfig))
	@stamp_tmp="$@.tmp.$$$$"; \
		printf '%s\n' $(notdir $(AUTOCONF_SRC)) > "$$stamp_tmp"; \
		mv -f "$$stamp_tmp" "$@"

pre-build-clean :
	@rm -rf $(AUTOCONF_DIR)

endif # AUTOCONF_MK
