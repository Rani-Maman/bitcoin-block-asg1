# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -fPIC
IDIR = -I.
OUTDIR ?= build

# Shared library setup
LIB = programs
LIB_SRC := programs_lib.cpp
LIBPATH := $(OUTDIR)/lib$(LIB).so

# Discover all .cpp files except the shared lib source
SRCS := $(filter-out $(LIB_SRC), $(wildcard *.cpp))

# Object files and executables
OBJS := $(addprefix $(OUTDIR)/, $(SRCS:.cpp=.o))
BINS := $(addprefix $(OUTDIR)/, $(SRCS:.cpp=.out))

# Default target
all: $(LIBPATH) $(BINS)

# Build shared library
$(LIBPATH): $(LIB_SRC)
	mkdir -p $(OUTDIR)
	$(CXX) -shared -o $@ $^ $(CXXFLAGS)

# Compile .cpp to .o
$(OUTDIR)/%.o: %.cpp
	mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(IDIR) -c -o $@ $<

# Link .cpp to .out
$(OUTDIR)/%.out: %.cpp $(LIBPATH)
	mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(IDIR) -o $@ $< -L$(OUTDIR) -l$(LIB) -Wl,-rpath=$(OUTDIR)

# Clean everything
clean:
	rm -rf $(OUTDIR)

