all:virusDetector

virusDetector: virusDetector.o 
	gcc -m32 -g -Wall -o virusDetector virusDetector.o 

virusDetector.o: virusDetector.c
	gcc -m32 -g -Wall -c -o virusDetector.o virusDetector.c


clean:
	rm -f *.o virusDetector