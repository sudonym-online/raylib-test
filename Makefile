CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Isrc -Iraylib/src -MMD -MP
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
  LDFLAGS = -Lraylib/build/raylib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
  LDFLAGS = -Lraylib/build/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lXcursor -lXinerama -lXrandr -lXi
endif

SRC = main.cpp src/objects.cpp src/physics.cpp src/player.cpp src/render.cpp $(wildcard objects/*/*.cpp)
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)
