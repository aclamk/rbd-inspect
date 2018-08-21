all: rbd-inspect rbd-iolog rbd-extract

clean:
	rm -f rbd-inspect.o model.o parse_log.o \
	rbd-inspect.o rbd-iolog.o rbd-extract.o \
	rbd-inspect rbd-iolog rbd-extract


rbd-inspect: parse_log.o model.o rbd-inspect.o
	g++ $^ -o $@

rbd-iolog: parse_log.o model.o rbd-iolog.o
	g++ $^ -o $@

rbd-extract: parse_log.o model.o rbd-extract.o
	g++ $^ -o $@

rbd-inspect.o: rbd-inspect.cpp Makefile
	g++ -g -Wall -c $< -o $@

rbd-iolog.o: rbd-iolog.cpp Makefile
	g++ -g -Wall -c $< -o $@

rbd-extract.o: rbd-extract.cpp Makefile
	g++ -g -Wall -c $< -o $@

model.o: model.cpp model.h Makefile
	g++ -g -Wall -c $< -o $@
	
parse_log.o: parse_log.cpp parse_log.h Makefile
	g++ -g -Wall  -c $< -o $@