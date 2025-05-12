
PWD=$(shell pwd)

.PHONY: shaders clean

shaders:
	"$(PWD)/shaders/build.sh" shaders/src bin/shaders
	"$(PWD)/shaders/build.sh" shaders/src/test bin/shaders/test

clean:
	rm -rf bin
