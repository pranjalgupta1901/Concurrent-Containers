all: mysort

elimination.o: elimination.cpp
	g++ -c elimination.cpp -O3 -std=c++20 -g -o elimination.o

M_and_S_queue.o: M_and_S_queue.cpp
	g++ -c M_and_S_queue.cpp -O3 -std=c++20 -g -o M_and_S_queue.o

Treiber_Stack.o: Treiber_Stack.cpp
	g++ -c Treiber_Stack.cpp -O3 -std=c++20 -g -o Treiber_Stack.o
    
SGL.o: SGL.cpp
	g++ -c SGL.cpp -O3 -std=c++20 -g -o SGL.o

flat_combining.o: flat_combining.cpp
	g++ -c flat_combining.cpp -O3 -std=c++20 -g -o flat_combining.o

spurious_wakeup.o: spurious_wakeup.cpp
	g++ -c spurious_wakeup.cpp -O3 -std=c++20 -g -o spurious_wakeup.o

mysort: mysort.cpp elimination.o M_and_S_queue.o Treiber_Stack.o SGL.o flat_combining.o spurious_wakeup.o
	g++ mysort.cpp elimination.o M_and_S_queue.o Treiber_Stack.o SGL.o flat_combining.o spurious_wakeup.o -O3 -std=c++20 -g -o mysort

.PHONY: clean
clean:
	rm -f *.o mysort
