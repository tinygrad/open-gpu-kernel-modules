NVIDIA_VERSION = 535.43.19

# This file.
VERSION_MK_FILE := $(lastword $(MAKEFILE_LIST))
$(OUTPUTDIR)/version.h: $(VERSION_MK_FILE)
	@$(MKDIR) $(OUTPUTDIR)
	@$(ECHO) '#define NVIDIA_VERSION "$(NVIDIA_VERSION)"' > $@

NV_GENERATED_HEADERS += $(OUTPUTDIR)/version.h
