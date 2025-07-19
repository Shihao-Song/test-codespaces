ROOT	:= ../../../drexel_llvm_course
SOURCE	:= $(ROOT)/loop_unroll/assignment/main.cc 
CC	:= clang++
FLAGS	:= -g -O3 -std=c++17 -w 
FLAGS	+= -I $(ROOT)
FLAGS	+= `llvm-config --cxxflags`
TARGET	:= main
LD	:= `llvm-config --ldflags --system-libs --libs core passes`
LD	+= `llvm-config --libs bitwriter`

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(FLAGS) $(SOURCE) -o $(TARGET) $(LD)

clean:
	rm -f $(TARGET)
