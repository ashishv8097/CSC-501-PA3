defrag:
	/usr/bin/gcc -o defrag -O0 -I. ./defrag.c

defrag: ./defrag.c ./defrag.h

clean:
	rm -rf `ls | grep -v 'README\|defrag.c\|defrag.h\|Makefile'`
