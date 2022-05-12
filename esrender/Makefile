thirdparty = ../thirdparty
util = ../util

bin = main

src = Pipelines.cpp \
	Render.cpp \
	RenderCore.cpp

src += $(util)/Controller.cpp \
	$(util)/Mesh.cpp \
	$(util)/Program.cpp

src += $(thirdparty)/imgui.cpp \
	$(thirdparty)/imgui_draw.cpp \
	$(thirdparty)/imgui_impl_glfw.cpp \
	$(thirdparty)/imgui_impl_opengl3.cpp \
	$(thirdparty)/imgui_tables.cpp \
	$(thirdparty)/imgui_widgets.cpp

src += $(thirdparty)/trackball.cc
src += $(thirdparty)/tiny_obj_loader.cc

obj = $(addsuffix .o, $(basename $(notdir $(src))))

CXXFLAGS = -std=c++11 -I. -I$(thirdparty) -I$(util) -Wall -Wformat -lglfw -lGLESv2 -ldl -lassimp -DES_BACKEND=1

CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(thirdparty)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(thirdparty)/%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(util)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(bin): $(obj)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -rf $(obj) $(bin)
