thirdparty = ../../thirdparty
util = ../util
bin = main

src = main.cpp
src += $(util)/Mesh.cpp
src += $(util)/Util.cpp
src += $(thirdparty)/tiny_obj_loader.cc

obj = $(addsuffix .o, $(basename $(notdir $(src))))

CXXFLAGS = -std=c++11 -I$(util) -I$(thirdparty) -I. -Wall -Wformat -lglfw -lGLEW -lGL -lSOIL

CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(util)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(thirdparty)/%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(bin): $(obj)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -rf $(obj) $(bin)
