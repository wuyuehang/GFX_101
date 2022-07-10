VK_SDK_PATH = /home/yuewu01/v/1.3.204.1/x86_64

CXXFLAGS = -g -std=c++11 -I$(VK_SDK_PATH)/include

LDFLAGS = -L$(VK_SDK_PATH)/lib -lvulkan

.PHONY: clean run all

binary = main

all: $(binary)

main : main.cpp
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

spv :
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.6 -V gl_SubgroupInvocationID.comp.glsl -o gl_SubgroupInvocationID.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.6 -V subgroupElect.comp.glsl -o subgroupElect.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.6 -V subgroupAll.comp.glsl -o subgroupAll.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.6 -V subgroupAny.comp.glsl -o subgroupAny.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.6 -V subgroupAllEqual.comp.glsl -o subgroupAllEqual.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.6 -V subgroupAdd.comp.glsl -o subgroupAdd.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.6 -V subgroupMax.comp.glsl -o subgroupMax.comp.spv

clean:
	rm -rf $(binary)
	rm -rf *.spv

run:
	LD_LIBRARY_PATH=$(VK_SDK_PATH)/lib VK_LAYER_PATH=$(VK_SDK_PATH)/etc/vulkan/explicit_layer.d \
	VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation ./main
