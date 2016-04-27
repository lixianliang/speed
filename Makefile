
default: build

clean: 
	rm -f objs/speed_* objs/dfs*
	rm -rf objs/src objs/test

build:
	sh auto/dir
	$(MAKE)	-f objs/Makefile_dfst
#	$(MAKE)	-f objs/Makefile_dfss;
	cp objs/dfs* bin/;
