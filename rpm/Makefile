VERSION=1.0.0.5

all : 
	$(MAKE) clean
	./createproj muduo
	cp muduo.spec muduo
	rm -rf ../../build/release
	(cd .. ; ./build.sh)
	./tar.sh
	./buildrpm muduo $(VERSION)

clean :
	rm -rf muduo
