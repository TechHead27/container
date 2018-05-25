CXX=g++
CXXFLAGS=-Wall -O
SOURCE=$(wildcard *.cpp)
OBJ=$(SOURCE:.cpp=.o)
EXECUTABLE=container

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $(EXECUTABLE) $(OBJ)

clean:
	rm -f $(EXECUTABLE) $(OBJ)
