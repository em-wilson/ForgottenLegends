all: tests

run: rom
	cd area && ../src/rom

rom:
	+$(MAKE) -C src

tests: rom
	+$(MAKE) -C test

test: tests
	test/tests

clean:
	+$(MAKE) clean -C src
	+$(MAKE) clean -C test
