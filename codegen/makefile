ROOT	:= ../../drexel_llvm_course
SOURCE	:= $(ROOT)/codegen/main.cc 
SOURCE	+= $(ROOT)/lexer/lexer.cc
SOURCE 	+= $(ROOT)/parser/parser.cc
SOURCE	+= $(ROOT)/codegen/codegen.cc
CC	:= clang++
FLAGS	:= -g -O3 -std=c++17 -w 
FLAGS	+= -I $(ROOT)
FLAGS	+= `llvm-config --cxxflags`
TARGET	:= codegen
LD	:= `llvm-config --ldflags --system-libs --libs core`
LD	+= `llvm-config --libs bitwriter`

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(FLAGS) $(SOURCE) -o $(TARGET) $(LD)

clean:
	rm -f $(TARGET) *bc
