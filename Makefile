all: rbd-inspect gen-iolog rbd-extract

clean:
	rm -f rbd-inspect.o model.o parse_log.o \
	rbd-inspect.o gen-iolog.o rbd-extract.o \
	rbd-inspect gen-iolog rbd-extract

OPTS = -ggdb3 -Wall -O3 -fno-omit-frame-pointer

rbd-inspect: parse_log.o model.o rbd-inspect.o
	g++ $^ -o $@

gen-iolog: parse_log.o model.o gen-iolog.o
	g++ $^ -o $@

rbd-extract: parse_log.o model.o rbd-extract.o
	g++ $^ -o $@

rbd-inspect.o: rbd-inspect.cpp Makefile
	g++ $(OPTS) -c $< -o $@

gen-iolog.o: gen-iolog.cpp Makefile
	g++ $(OPTS) -c $< -o $@

rbd-extract.o: rbd-extract.cpp Makefile
	g++ $(OPTS) -c $< -o $@

model.o: model.cpp model.h Makefile
	g++ $(OPTS) -c $< -o $@
	
parse_log.o: parse_log.cpp parse_log.h Makefile
	g++ $(OPTS) -c $< -o $@