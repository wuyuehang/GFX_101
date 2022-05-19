thirdparty = ../../thirdparty
util = ../../util
helper = ../helper
bin = main

src = main.cpp
src += $(util)/Controller.cpp \
	$(util)/AssimpMesh.cpp \
	$(util)/Program.cpp

src += $(helper)/Util.cpp \

src += $(thirdparty)/tiny_obj_loader.cc \
	$(thirdparty)/trackball.cc

obj = $(addsuffix .o, $(basename $(notdir $(src))))

CXXFLAGS = -std=c++11 -I$(helper) -I$(util) -I$(thirdparty) -I. -Wall -Wformat -lglfw -lGLEW -lGL -DGL_BACKEND=1 -lassimp

CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(helper)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(util)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(thirdparty)/%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(bin): $(obj)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -rf $(obj) $(bin)
