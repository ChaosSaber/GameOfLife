# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS2  = -g -Wall -std=c99 -lc -D _BSD_SOURCE -fopenmp -lm
CFLAGS = -lm `pkg-config --libs --cflags opencv` -ldl -g -Wall -std=c99 -lc -D _BSD_SOURCE -fopenmp 

# the build target executable:
TARGET = gameoflifeparallel
TARGET2 = gameoflife

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c pictureloader.c



run: all
	OMP_NUM_THREADS=6 ./$(TARGET)
	
clean:
	$(RM) $(TARGET)
