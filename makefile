build: scry.h scry.cc scry_user.cc
	g++ -fPIC -shared scry.cc -o scry.so
	g++ scry_user.cc -ldl -o scry_user
