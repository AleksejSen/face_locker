VERSION := 1.0.0
APP_NAME := face_locker
IS_SERVICE := false

# Detect number of processors for parallel testing
JOBS := $(shell nproc 2>/dev/null || echo 1)
CHECK_STATUS = && echo "\n === RECOGNITION SUCCESS ===" || echo "\n=== RECOGNITION FAILED ==="

# Convert APP_NAME to lowercase and change underscores to dashes for Debian compliance
DEB_PKG_NAME := $(shell echo "$(APP_NAME)" | tr '[:upper:]' '[:lower:]' | tr '_' '-')
DEB_NAME := $(DEB_PKG_NAME)_$(VERSION)_amd64.deb

all: build

build:
	git submodule update --init --recursive
	cmake -S . -B build \
		-DCMAKE_BUILD_TYPE=Debug \
		-DMAKE_APP_NAME=$(APP_NAME) \
		-DMAKE_VERSION=$(VERSION)
	cmake --build build -j$(JOBS)
	ln -sf build/compile_commands.json .

run_false:
	@./build/bin/$(APP_NAME) \
		--reference_picture pictures/rambo.jpg \
		--debug \
		$(CHECK_STATUS)

run:
	@./build/bin/$(APP_NAME) \
		--reference_picture pictures/my_pic.jpeg \
		$(CHECK_STATUS)

run_multi:
	@./build/bin/$(APP_NAME) \
		--reference_picture pictures/family.jpeg \
		--debug \
		$(CHECK_STATUS)

test:
	@echo "Running tests in parallel using $(JOBS) jobs..."
	@cd build && ctest -j$(JOBS) --output-on-failure

build-deb: build
	@echo "Syncing structural layout from manual deb-template folder..."
	rm -rf deb-package
	@# Create basic control folder structure if it doesn't exist
	mkdir -p deb-package/DEBIAN
	@if [ -d deb-template ]; then cp -r deb-template/* deb-package/ 2>/dev/null || true; fi
	@# Generate default control file if fallback is required
	@if [ ! -f deb-package/DEBIAN/control ]; then \
		echo "Package: $(DEB_PKG_NAME)" > deb-package/DEBIAN/control; \
		echo "Section: utils" >> deb-package/DEBIAN/control; \
		echo "Priority: optional" >> deb-package/DEBIAN/control; \
		echo "Architecture: amd64" >> deb-package/DEBIAN/control; \
		echo "Maintainer: Developer <maintainer@example.com>" >> deb-package/DEBIAN/control; \
		echo "Description: Automated Debian package for $(APP_NAME)." >> deb-package/DEBIAN/control; \
	fi
	@echo "AUTOMATION FIX: Dynamically updating version to $(VERSION) in control file..."
	@sed -i "s|^Version:.*|Version: $(VERSION)|" deb-package/DEBIAN/control || echo "Version: $(VERSION)" >> deb-package/DEBIAN/control
	@echo "Ensuring package directories are generated correctly..."
	mkdir -p deb-package/usr/bin
	@echo "Injecting freshly compiled application binary into package..."
	cp build/bin/$(APP_NAME) deb-package/usr/bin/
	chmod 755 deb-package/usr/bin/$(APP_NAME)
	@# AUTOMATED SERVICE INJECTION: Generates the .service layout and controls on the fly
	@if [ "$(IS_SERVICE)" = "true" ]; then \
		echo "Ensuring target systemd installation directory layout exists..."; \
		mkdir -p deb-package/lib/systemd/system; \
		echo "Generating systemd service configuration on the fly..."; \
		echo "[Unit]" > deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "Description=$(APP_NAME) System Daemon Service" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "After=network.target" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "[Service]" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "Type=simple" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "ExecStart=/usr/bin/$(APP_NAME)" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "Restart=on-failure" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "RestartSec=5" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "[Install]" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		echo "WantedBy=multi-user.target" >> deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		chmod 644 deb-package/lib/systemd/system/$(DEB_PKG_NAME).service; \
		\
		echo "Injecting Service installation hooks (postinst / prerm)..."; \
		echo "#!/bin/sh" > deb-package/DEBIAN/postinst; \
		echo "systemctl daemon-reload" >> deb-package/DEBIAN/postinst; \
		echo "systemctl enable $(DEB_PKG_NAME).service" >> deb-package/DEBIAN/postinst; \
		echo "systemctl start $(DEB_PKG_NAME).service" >> deb-package/DEBIAN/postinst; \
		chmod 755 deb-package/DEBIAN/postinst; \
		\
		echo "#!/bin/sh" > deb-package/DEBIAN/prerm; \
		echo "systemctl stop $(DEB_PKG_NAME).service" >> deb-package/DEBIAN/prerm; \
		echo "systemctl disable $(DEB_PKG_NAME).service" >> deb-package/DEBIAN/prerm; \
		chmod 755 deb-package/DEBIAN/prerm; \
	fi
	@echo "Ensuring output build directory exists..."
	mkdir -p build
	@echo "Assembling fully self-contained Debian distribution file into build folder..."
	dpkg-deb --root-owner-group --build deb-package build/$(DEB_NAME)
	@echo "Debian bundle updated successfully: build/$(DEB_NAME)"

install-deb: build-deb
	@echo "Deploying Debian application architecture package via system permissions..."
	sudo dpkg -i build/$(DEB_NAME)

uninstall-deb:
	@echo "Purging deployed package binaries from host operating system environment..."
	sudo dpkg -P $(DEB_PKG_NAME)

clean:
	rm -rf build deb-package compile_commands.json

.PHONY: all build run run_false run_multi test clean build-deb install-deb uninstall-deb
