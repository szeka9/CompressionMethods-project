CC = g++
IDIR =../include
CFLAGS = -g -Wall -Ofast -fopenmp -fopt-info-vec -I$(IDIR)

ODIR = obj
LDIR =../lib

_DEPS = BinaryUtils.hh HuffmanTransducer.hh MarkovEncoder.hh IEncoder.hh EncoderChain.hh Padder.hh
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o BinaryUtils.o HuffmanTransducer.o MarkovEncoder.o EncoderChain.o Padder.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_T_OBJ = testcases.o BinaryUtils.o HuffmanTransducer.o MarkovEncoder.o EncoderChain.o Padder.o
T_OBJ = $(patsubst %,$(ODIR)/%,$(_T_OBJ))

MKDIR_P = mkdir -p

$(ODIR)/%.o: %.cc $(DEPS)
	$(MKDIR_P) $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

HuffmanTransducer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

TestCases: $(T_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o