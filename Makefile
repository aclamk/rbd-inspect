all: rbd-inspect

clean:
	rm -f rbd-inspect.o model.o parse_log.o rbd-inspect


rbd-inspect: parse_log.o model.o rbd-inspect.o
	g++ $^ -o $@

rbd-inspect.o: rbd-inspect.cpp Makefile
	g++ -g -c $< -o $@

model.o: model.cpp model.h Makefile
	g++ -g -c $< -o $@
	
parse_log.o: parse_log.cpp parse_log.h Makefile
	g++ -g -c $< -o $@