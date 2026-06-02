.PHONY: help configure build run rebuild clean test-webhook test-uri test-get \
	install-service uninstall-service service-status service-logs

LISTEN ?=
TOKEN ?= test-token

BUILD_DIR ?= build
PORT ?= 48080
TARGET := cxx_chatwork
BINARY := $(BUILD_DIR)/$(TARGET)

SERVICE := cxx-chatwork
UNIT_FILE := /etc/systemd/system/$(SERVICE).service

help:
	@printf '%s\n' \
		'Targets:' \
		'  make build           Configure and build the executable' \
		'  make run             Run with CXX_CHATWORK_PORT=$${PORT:-8080}' \
		'  make clean           Remove build artifacts' \
		'  make rebuild         Clean, configure, and build' \
		'  make configure       Generate CMake build files' \
		'  make test-webhook    POST a Slack Outgoing Webhook payload containing a URL' \
		'  make test-uri        POST a Slack Outgoing Webhook payload with multiple URLs' \
		'  make test-get        GET a bookmark for a URL from the running server' \
		'  make install-service Install and reload the systemd unit (uses sudo)' \
		'  make uninstall-service Stop, disable and remove the systemd unit (uses sudo)' \
		'  make service-status  Show the systemd service status' \
		'  make service-logs    Follow the systemd service logs' \
		'' \
		'Variables:' \
		'  PORT=48080           Port used by run and test targets' \
		'  TOKEN=test-token     Slack Outgoing Webhook token used by run and test targets' \
		'  BUILD_DIR=build      CMake build directory'

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	CXX_CHATWORK_PORT=$(PORT) CXX_CHATWORK_LISTEN=$(LISTEN) SLACK_OUTGOING_TOKEN=$(TOKEN) $(BINARY)

rebuild: clean build

clean:
	cmake -E remove_directory $(BUILD_DIR)

test-webhook:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/cxx_chatwork \
		--data-urlencode 'token=$(TOKEN)' \
		--data-urlencode 'user_id=U07TAKANO32' \
		--data-urlencode 'text=PR を確認してください https://github.com/takano32/brevaluck/pulls'

test-uri:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/cxx_chatwork \
		--data-urlencode 'token=$(TOKEN)' \
		--data-urlencode 'user_id=U07TAKANO32' \
		--data-urlencode 'text=参考: https://github.com/takano32/brevaluck/ と https://developer.hatena.ne.jp/ を見てください。'

test-get:
	curl -sS -i -G http://127.0.0.1:$(PORT)/bookmark \
		--data-urlencode 'url=https://github.com/takano32/brevaluck/'

install-service:
	sed -e 's|__USER__|$(USER)|g' -e 's|__WORKDIR__|$(CURDIR)|g' deploy/$(SERVICE).service \
		| sudo tee $(UNIT_FILE) > /dev/null
	sudo systemctl daemon-reload
	@echo 'Installed $(UNIT_FILE)'
	@echo 'Enable and start with: sudo systemctl enable --now $(SERVICE)'

uninstall-service:
	-sudo systemctl disable --now $(SERVICE)
	sudo rm -f $(UNIT_FILE)
	sudo systemctl daemon-reload

service-status:
	systemctl status $(SERVICE)

service-logs:
	journalctl -u $(SERVICE) -f
