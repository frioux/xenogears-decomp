# Names and Paths
BUILD_DIR    := build
CONFIG_DIR   := config
TOOLS_DIR    := tools
OBJDIFF_DIR  := $(TOOLS_DIR)/objdiff
EXPECTED_DIR := expected
DOCKERFILE   := ./Dockerfile
DOCKER       := $(shell sh -c 'command -v docker || true')
OBJDIFF      := $(OBJDIFF_DIR)/objdiff
# prefer the repo venv when it is runnable on this host (inside the container it is not)
PYTHON       := $(shell sh -c 'test -x .venv/bin/python3 && .venv/bin/python3 -c "" 2>/dev/null && echo .venv/bin/python3 || echo python3')
SHA256SUM    := $(shell sh -c 'command -v sha256sum || echo "shasum -a 256"')
# only allocate a TTY for the container when we have one (CI / agents often do not)
DOCKER_TTY   := $(shell sh -c '[ -t 0 ] && echo -it || echo -i')

# Detect host OS and architecture
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M := $(shell sh -c 'uname -m 2>/dev/null || echo not')

# Tools
# The binary tools in this repo require linux/x86_64 -- if we're on that platform,
# use prebuilt Gears and do build directly on the host.
# Edge-cases may require building Gears from source on linux/x86_64 (ex: mismatched GLIBC version)
# so we default to the locally built binary if it is present
ifeq ($(uname_S)-$(uname_M),Linux-x86_64)
	GEARS_LOCAL  := $(TOOLS_DIR)/gears/target/release/gears
ifneq (,$(shell sh -c '$(GEARS_LOCAL) --version 2>/dev/null || true'))
	GEARS        := $(GEARS_LOCAL)
ifneq (,$(shell sh -c 'command -v cargo || true'))
	# keeps the local build up to date with its "sources" for convenience, if cargo is available
	# (not actual versioning, just file mod_date for now)
	NEED_GEARS := true
else
	NEED_GEARS := false
endif
else
	GEARS        := $(TOOLS_DIR)/gears/prebuilt/gears
	NEED_GEARS := false
endif
	NEED_DOCKER := false
else
	# Use the compiled version of Gears
	GEARS        := $(TOOLS_DIR)/gears/target/release/gears
	# flag used below to run build in a docker container
	NEED_DOCKER := true
	# flag used below to trigger gears build automatically
	NEED_GEARS := true
endif

# Settings
ifeq ($(uname_S),Linux)
	NUMPROC ?= $(shell nproc)
endif
ifeq ($(uname_S),Darwin)
	NUMPROC ?= $(shell sysctl -n hw.logicalcpu)
endif

# Rules
default: all

all: build

ifeq ($(NEED_GEARS),true)
# add make targets to build Gears, and add dependencies.
GEARS_SRC := $(TOOLS_DIR)/gears/Cargo.toml $(TOOLS_DIR)/gears/Cargo.lock $(wildcard $(TOOLS_DIR)/gears/src/*.rs)
$(GEARS): $(GEARS_SRC)
	cd $(TOOLS_DIR)/gears && cargo build --release
	#timestamp trick (sorry, have not yet found a better way to do this)
	touch $@

ifneq ($(NEED_DOCKER), true)
build: $(GEARS)
endif
clean: $(GEARS)
endif

ifeq ($(NEED_DOCKER),true)
# provide a useful error for non-linux/x86_64 platforms
ifeq (,$(DOCKER))
$(error Docker is required for platform $(uname_S)/$(uname_M))
endif

# make target to build the container
docker:
	docker build --platform linux/amd64 . -t ethos:latest --quiet
# These targets run inside the container; files get written to the host
# filesystem (the repo is bind-mounted), not the container filesystem.
build objdiff-config report: docker
	docker run --rm $(DOCKER_TTY) --platform=linux/amd64 -v$(PWD):/xenogears-decomp ethos:latest /bin/bash -c '. /.venv/bin/activate && make $@'
# Open a shell in the build container with the repo mounted, for running ninja /
# objdiff / objdump by hand.
shell: docker
	docker run --rm -it --platform=linux/amd64 -v$(PWD):/xenogears-decomp ethos:latest /bin/bash -c '. /.venv/bin/activate && exec bash'
else
build:
	$(MAKE) clean; \
	$(GEARS) matching; \
	ninja -t clean; \
	ninja -j$(NUMPROC)

objdiff-config:
	$(MAKE) clean; \
	$(GEARS) report; \
	ninja -t clean; \
	ninja -j$(NUMPROC); \
	mkdir -p $(EXPECTED_DIR); \
	mv build/asm $(EXPECTED_DIR)/asm; \
	$(PYTHON) $(OBJDIFF_DIR)/objdiff_generate.py $(OBJDIFF_DIR)/config.yaml

report: objdiff-config
	@$(OBJDIFF) report generate > $(BUILD_DIR)/progress.json
endif

check: clean build
	@$(SHA256SUM) --check $(CONFIG_DIR)/checksum.sha

# Static progress site (build/progress-site/index.html) from the objdiff report.
progress: report
	$(PYTHON) tools/progress/render.py --report $(BUILD_DIR)/progress.json --out $(BUILD_DIR)/progress-site
	@echo "progress site: $(BUILD_DIR)/progress-site/index.html"

# Render and push the progress site (with history) to the gh-pages branch.
progress-publish: report
	tools/progress/publish.sh $(BUILD_DIR)/progress.json

clean:
	@$(GEARS) clean; \
	rm -rf $(EXPECTED_DIR)

# Pull SLUS_006.64 and the overlays out of a raw MODE2/2352 disc 1 image into disc/.
# Usage: make extract DISC=/path/to/Xenogears\ \(USA\)\ \(Disc\ 1\).bin
# Defaults to the first .bin found under disc/disc1/.
DISC ?= $(shell sh -c 'ls disc/disc1/*/*.bin disc/disc1/*.bin 2>/dev/null | head -1')
extract:
	@test -n "$(DISC)" || { echo "DISC=<path to disc 1 .bin> is required"; exit 1; }
	$(PYTHON) tools/scripts/extract_exe.py "$(DISC)"
	$(PYTHON) tools/scripts/extract_overlays.py "$(DISC)"
	$(PYTHON) tools/scripts/verify_disc.py

### Settings
.SECONDARY:
.PHONY: all clean default extract docker shell build check objdiff-config report progress progress-publish
SHELL = /bin/bash -e -o pipefail
