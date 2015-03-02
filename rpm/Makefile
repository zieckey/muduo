VERSION=1.0.0.8

all : 
	$(MAKE) clean
	./createproj muduo
	cp muduo.spec muduo
	rm -rf ../../build/release
	(cd .. ; ./build.sh)
	(cd .. ; sh build.sh)
	./tar.sh
	./buildrpm muduo $(VERSION)

clean :
	rm -rf muduo
