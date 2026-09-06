.PHONY: all native armv7 arm64 amd64 musl test clean

all:
	./build.sh all

native:
	./build.sh native

armv7:
	./build.sh armv7

arm64:
	./build.sh arm64

amd64:
	./build.sh amd64

musl: armv7 arm64 amd64

test: native
	./tests/unit/test_unit_security.sh
	./tests/unit/test_unit_helpers.sh

clean:
	rm -rf bin/

