VK_SDK_PATH = /home/yuewu01/v/1.3.204.1/x86_64

thirdparty = ./thirdparty
shaders = ./shaders

CXXFLAGS = -std=c++11 -I$(VK_SDK_PATH)/include -I. -I$(thirdparty) -Wall -Wformat
LDFLAGS = -L$(VK_SDK_PATH)/lib -lvulkan `pkg-config --libs glfw3` -lSOIL

.PHONY: clean run all

binary = main

src = Axis.cpp \
	Controller.cpp \
	Default.cpp \
	HelloVulkan.cpp \
	HelloVulkanCore.cpp \
	Mesh.cpp \
	Phong.cpp \
	Visualize_Vertex_Normal.cpp \
	VulkanCommon.cpp \
	VulkanSwapchain.cpp \
	Wireframe.cpp

src += $(thirdparty)/imgui.cpp \
	$(thirdparty)/imgui_draw.cpp \
	$(thirdparty)/imgui_impl_glfw.cpp \
	$(thirdparty)/imgui_impl_vulkan.cpp \
	$(thirdparty)/imgui_tables.cpp \
	$(thirdparty)/imgui_widgets.cpp

src += $(thirdparty)/trackball.cc
src += $(thirdparty)/tiny_obj_loader.cc

obj += $(addsuffix .o, $(basename $(notdir $(src))))

all: $(binary)

$(binary) : $(obj)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o : $(thirdparty)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o : $(thirdparty)/%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

spv :
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/axis.vert -o $(shaders)/axis.vert.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/axis.frag -o $(shaders)/axis.frag.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/simple.vert -o $(shaders)/simple.vert.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/combined_image_sampler.frag -o $(shaders)/combined_image_sampler.frag.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/wireframe.frag -o $(shaders)/wireframe.frag.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/visualize_vertex_normal.vert -o $(shaders)/visualize_vertex_normal.vert.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/visualize_vertex_normal.geom -o $(shaders)/visualize_vertex_normal.geom.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/visualize_vertex_normal.frag -o $(shaders)/visualize_vertex_normal.frag.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/phong.vert -o $(shaders)/phong.vert.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V $(shaders)/phong.frag -o $(shaders)/phong.frag.spv

clean:
	rm -rf $(binary) $(obj)

run:
	LD_LIBRARY_PATH=$(VK_SDK_PATH)/lib VK_LAYER_PATH=$(VK_SDK_PATH)/etc/vulkan/explicit_layer.d \
	VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation ./main
