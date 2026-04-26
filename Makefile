.PHONY: setup test train-smoke

setup:
	cmake -S . -B build

test:
	cmake --build build --config Release

train-smoke:
	cmake --build build --config Release
