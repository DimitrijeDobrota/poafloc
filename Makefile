
all: demo

demo: demo.cpp args.hpp
	g++ -o $@ $< -std=c++20 -Wall -Werror -ggdb

clean:
	rm -rf demo


.PHONY: clean
