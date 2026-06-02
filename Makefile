.PHONY: help configure build run rebuild clean test-webhook test-challenge test-uri test-get \
	install-service uninstall-service service-status service-logs

LISTEN ?=

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
		'  make test-webhook    POST a Slack message containing URLs to the running server' \
		'  make test-challenge  POST a Slack URL verification payload' \
		'  make test-uri        POST a Slack message with multiple URLs to the running server' \
		'  make test-get        GET a bookmark for a URL from the running server' \
		'  make install-service Install and reload the systemd unit (uses sudo)' \
		'  make uninstall-service Stop, disable and remove the systemd unit (uses sudo)' \
		'  make service-status  Show the systemd service status' \
		'  make service-logs    Follow the systemd service logs' \
		'' \
		'Variables:' \
		'  PORT=48080           Port used by run and test targets' \
		'  BUILD_DIR=build      CMake build directory'

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	CXX_CHATWORK_PORT=$(PORT) CXX_CHATWORK_LISTEN=$(LISTEN) $(BINARY)

rebuild: clean build

clean:
	cmake -E remove_directory $(BUILD_DIR)

test-webhook:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/slack \
		-H 'Content-Type: application/json' \
		--data '{"event":{"type":"message","user":"U07TAKANO32","text":"PR を確認してください https://github.com/takano32/brevaluck/pulls"}}'

test-challenge:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/slack \
		-H 'Content-Type: application/json' \
		--data '{"type":"url_verification","challenge":"challenge-token"}'

test-uri:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/slack \
		-H 'Content-Type: application/json' \
		--data '{"event":{"type":"message","user":"U07TAKANO32","text":"参考: https://github.com/takano32/brevaluck/ と https://developer.hatena.ne.jp/ を見てください。"}}'

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
