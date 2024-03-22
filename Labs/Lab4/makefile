all: task1 task4
task1: task1.c
	gcc -m32 -o task1 task1.c

task4: task4.c
	gcc -m32 -fno-pie -fno-stack-protector -o task4 task4.c

clean:
	rm -f task1 task4
	