.PHONY: help configure build run rebuild clean test-webhook test-challenge test-uri test-get

LISTEN ?=

BUILD_DIR ?= build
PORT ?= 48080
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
		'  make test-get        GET a bookmark for a URL from the running server' \
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
