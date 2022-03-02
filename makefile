build: src/*.cc
	g++ -g -Og -D DEBUG -rdynamic -std=c++20 -fPIC -shared src/*.cc -o libscry.so

release: src/*.cc
	g++ -std=c++20 -fPIC -shared src/*.cc -o libscry.so

install: src/*.h libscry.so
	mkdir -p /usr/include/scry
	mkdir -p /usr/lib/
	cp src/*.h /usr/include/scry/
	mv libscry.so /usr/lib/libscry.so

clean:
	rm -f libscry.so
