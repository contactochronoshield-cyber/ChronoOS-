.PHONY: all clean init chronod chronoctl

all:
	@echo "======================================="
	@echo "      Building ChronoOS 1.0.0-alpha"
	@echo "======================================="
	@$(MAKE) -C init
	@$(MAKE) -C services/chronod
	@$(MAKE) -C cli/chronoctl
	@echo ""
	@echo "Build completed successfully."

init:
	@$(MAKE) -C init

chronod:
	@$(MAKE) -C services/chronod

chronoctl:
	@$(MAKE) -C cli/chronoctl

clean:
	@$(MAKE) -C init clean
	@$(MAKE) -C services/chronod clean
	@$(MAKE) -C cli/chronoctl clean
