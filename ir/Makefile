LLVM_LIB_PATH = /home/yuewu01/llvm-project/build/lib
LLVM_INC_PATH = /home/yuewu01/llvm-project/llvm/include

CXXFLAGS = `/home/yuewu01/llvm-project/build/bin/llvm-config --cxxflags`
CXXFLAGS += -g

LDFLAGS = `/home/yuewu01/llvm-project/build/bin/llvm-config --ldflags`
#LDFLAGS += `/home/yuewu01/llvm-project/build/bin/llvm-config --system-libs`
LDFLAGS += -lpthread -ltinfo
#LDFLAGS += `/home/yuewu01/llvm-project/build/bin/llvm-config --libs`
LDFLAGS += -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMBinaryFormat -lLLVMSupport

.PHONY: clean run all

binary = Basic GEP

all: $(binary)

Basic : Basic.cpp
	clang++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

GEP : GEP.cpp
	clang++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(binary)

run:
	$(foreach exe, $(binary), ./$(exe);)
