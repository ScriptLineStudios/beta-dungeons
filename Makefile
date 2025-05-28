all: beta_dungeons.o example

beta_dungeons.o: src/beta_dungeons.cpp
	g++ -c -o beta_dungeons.o src/beta_dungeons.cpp -O3

example: example.cpp
	g++ -o example example.cpp beta_dungeons.o -O3

clean:
	rm beta_dungeons.o example