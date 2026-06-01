.PHONY: help configure build run rebuild clean test-webhook test-challenge test-uri

BUILD_DIR ?= build
PORT ?= 8080
TARGET := cxx_chatwork
BINARY := $(BUILD_DIR)/$(TARGET)

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
		'' \
		'Variables:' \
		'  PORT=18080           Port used by run and test targets' \
		'  BUILD_DIR=build      CMake build directory'

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	CXX_CHATWORK_PORT=$(PORT) $(BINARY)

rebuild: clean build

clean:
	cmake -E remove_directory $(BUILD_DIR)

test-webhook:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/slack \
		-H 'Content-Type: application/json' \
		--data '{"event":{"type":"message","text":"PR を確認してください https://github.com/org/repo/pull/1"}}'

test-challenge:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/slack \
		-H 'Content-Type: application/json' \
		--data '{"type":"url_verification","challenge":"challenge-token"}'

test-uri:
	curl -sS -i -X POST http://127.0.0.1:$(PORT)/slack \
		-H 'Content-Type: application/json' \
		--data '{"event":{"type":"message","text":"参考: https://example.com/docs と https://github.com/org/repo を見てください。"}}'
