CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -I ../firmware/lib/vehicle_core/include
SRC := \
	../firmware/lib/vehicle_core/src/heading.cpp \
	../firmware/lib/vehicle_core/src/control.cpp \
	../firmware/lib/vehicle_core/src/color.cpp \
	../firmware/lib/vehicle_core/src/protocol.cpp \
	../firmware/lib/vehicle_core/src/encoder.cpp
OUT := test_vehicle_core

.PHONY: test clean

test: $(OUT)
	./$(OUT)

$(OUT): test_vehicle_core.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ test_vehicle_core.cpp $(SRC)

clean:
	rm -f $(OUT)
