
all : 
	$(MAKE) clean
	./createproj muduo
	cp muduo.spec muduo
	./tar.sh
	./buildrpm muduo 1.0.0.4

clean :
	rm -rf muduo
