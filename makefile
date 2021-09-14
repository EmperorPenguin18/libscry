build: libscry.h libscry.cc scry_user.cc
	g++ -lcurl -fPIC -shared libscry.cc -o libscry.so
	g++ scry_user.cc -ldl -o scry_user

install: libscry.h libscry.so scry_user
	@mkdir -p /usr/include/
	@mkdir -p /usr/lib/
	@mkdir -p /usr/bin/
	@cp libscry.h /usr/include/libscry.h
	@mv libscry.so /usr/lib/libscry.so
	@mv scry_user /usr/bin/scry_user
