thirdparty = ../thirdparty
bin = main

src = main.cpp

src += $(thirdparty)/imgui.cpp \
	$(thirdparty)/imgui_draw.cpp \
	$(thirdparty)/imgui_impl_glfw.cpp \
	$(thirdparty)/imgui_impl_opengl3.cpp \
	$(thirdparty)/imgui_tables.cpp \
	$(thirdparty)/imgui_widgets.cpp

obj = $(addsuffix .o, $(basename $(notdir $(src))))

CXXFLAGS = -std=c++11 -I$(thirdparty) -I. -Wall -Wformat -lglfw -lGLEW -lGL -DGL_BACKEND=1 -ldl

CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(thirdparty)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(bin): $(obj)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -rf $(obj) $(bin)
