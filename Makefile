all: rbd-model gen-iolog rbd-iolog rbd-record

clean:
	rm -f rbd-model.o model.o parse_log.o \
	rbd-model.o gen-iolog.o rbd-iolog.o rbd-record.o \
	rbd-model gen-iolog rbd-iolog rbd-record

OPTS = -ggdb3 -Wall -O3 -fno-omit-frame-pointer

rbd-record: parse_log.o model.o rbd-record.o
	g++ $^ -o $@

rbd-model: parse_log.o model.o rbd-model.o
	g++ $^ -o $@

gen-iolog: parse_log.o model.o gen-iolog.o
	g++ $^ -o $@

rbd-iolog: parse_log.o model.o rbd-iolog.o
	g++ $^ -o $@

rbd-record.o: rbd-record.cpp Makefile
	g++ $(OPTS) -c $< -o $@

rbd-model.o: rbd-model.cpp Makefile
	g++ $(OPTS) -c $< -o $@

gen-iolog.o: gen-iolog.cpp Makefile
	g++ $(OPTS) -c $< -o $@

rbd-iolog.o: rbd-iolog.cpp Makefile
	g++ $(OPTS) -c $< -o $@

model.o: model.cpp model.h Makefile
	g++ $(OPTS) -c $< -o $@
	
parse_log.o: parse_log.cpp parse_log.h Makefile
	g++ $(OPTS) -c $< -o $@