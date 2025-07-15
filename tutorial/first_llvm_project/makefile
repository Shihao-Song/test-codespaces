LLVM_CONFIG := llvm-config
LDFLAGS     := $(shell $(LLVM_CONFIG) --ldflags) $(shell $(LLVM_CONFIG) --libs bitreader core support) -pthread
COMMON_FLAGS:= -Wall -Wextra
CXXFLAGS    := $(COMMON_FLAGS) $(shell $(LLVM_CONFIG) --cxxflags)
CPPFLAGS    := $(shell $(LLVM_CONFIG) --cppflags)
TARGET      := hello

all:
	g++ $(TARGET).cc $(CPPFLAGS) $(CXXFLAGS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

