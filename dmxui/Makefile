SOURCES := $(shell find -type f)
all: dist

.PHONY: clean
clean:
	npm run clean

.PHONY: install
install:
	npm install

dist: install ${SOURCES}
	npm run build
