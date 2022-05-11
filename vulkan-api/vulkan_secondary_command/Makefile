VK_SDK_PATH = /home/yuewu01/v/1.3.204.1/x86_64

thirdparty = ../../thirdparty

CXXFLAGS = -g -std=c++11 -I$(VK_SDK_PATH)/include -I$(thirdparty)

LDFLAGS = -L$(VK_SDK_PATH)/lib -lvulkan `pkg-config --libs glfw3`

.PHONY: clean run all

binary = main

all: $(binary)

main : main.cpp
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

spv :
	$(VK_SDK_PATH)/bin/glslangValidator -V simple.vert.glsl -o simple.vert.spv
	$(VK_SDK_PATH)/bin/glslangValidator -V combined_image_sampler.frag.glsl -o combined_image_sampler.frag.spv

clean:
	rm -rf $(binary)
	rm -rf *.spv

run:
	LD_LIBRARY_PATH=$(VK_SDK_PATH)/lib VK_LAYER_PATH=$(VK_SDK_PATH)/etc/vulkan/explicit_layer.d \
	VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation ./main
