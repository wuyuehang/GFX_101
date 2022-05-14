thirdparty = ../../thirdparty
help = ../helper
bin = main

src = main.cpp

src += $(help)/Util.cpp

obj = $(addsuffix .o, $(basename $(notdir $(src))))

CXXFLAGS = -std=c++11 -I$(help) -I$(thirdparty) -I. -Wall -Wformat -lglfw -lGLEW -lGL

CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(help)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(bin): $(obj)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -rf $(obj) $(bin)
