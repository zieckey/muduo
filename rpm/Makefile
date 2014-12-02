VERSION=1.0.0.6

all : 
	$(MAKE) clean
	./createproj muduo
	cp muduo.spec muduo
	rm -rf ../../build/release
	(cd .. ; ./build.sh)
	(cd .. ; export BUILD_TYPE=debug ; sh build.sh)
	./tar.sh
	./buildrpm muduo $(VERSION)

clean :
	rm -rf muduo
