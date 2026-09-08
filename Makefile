# Makefile — see docs/BUILD-RULES.md (macOS 26, WEG absent)

.PHONY: test kext clean

test:
	$(MAKE) -C ihv/amd-rdna2-igpu test

kext:
	$(MAKE) -C ihv/amd-rdna2-igpu kext

clean:
	$(MAKE) -C ihv/amd-rdna2-igpu clean
