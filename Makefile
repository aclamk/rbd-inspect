all: rbd-inspect




rbd-inspect: parse_log.o model.o

model.o: model.cpp model.h Makefile
	g++ -c $< -o $@
	
parse_log.o: parse_log.cpp parse_log.h Makefile
	g++ -c $< -o $@