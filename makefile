build: libscry.h libscry.cc
	g++ -lcurl -fPIC -shared libscry.cc -o libscry.so

install: libscry.h libscry.so
	@mkdir -p /usr/include/
	@mkdir -p /usr/lib/
	@cp libscry.h /usr/include/libscry.h
	@mv libscry.so /usr/lib/libscry.so
