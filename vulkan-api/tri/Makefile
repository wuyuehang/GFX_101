VK_SDK_PATH = /home/yuewu01/v/1.3.216.0/x86_64

common = ../../common/vulkan
thirdparty = ../../thirdparty

CXXFLAGS = -std=c++11 -I$(VK_SDK_PATH)/include -I. -I$(common) -I$(thirdparty) -Wall -Wformat
LDFLAGS = -L$(VK_SDK_PATH)/lib -lvulkan `pkg-config --libs glfw3`

.PHONY: clean run all

binary = main

src = main.cpp
src += $(common)/VulkanCore.cpp \
	$(common)/VulkanCommon.cpp \
	$(common)/VulkanSwapchain.cpp

obj += $(addsuffix .o, $(basename $(notdir $(src))))

all: $(binary)

$(binary) : $(obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(common)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

spv :
	$(VK_SDK_PATH)/bin/glslangValidator -V simple.vert -o simple.vert.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V simple.frag -o simple.frag.spv

clean:
	rm -rf $(binary) $(obj) *.spv

run:
	LD_LIBRARY_PATH=$(VK_SDK_PATH)/lib VK_LAYER_PATH=$(VK_SDK_PATH)/etc/vulkan/explicit_layer.d \
	VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation ./main
