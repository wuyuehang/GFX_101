VK_SDK_PATH = /home/yuewu01/v/1.3.204.1/x86_64

CXXFLAGS = -g -std=c++11 -I$(VK_SDK_PATH)/include

LDFLAGS = -L$(VK_SDK_PATH)/lib -lvulkan

.PHONY: clean run all

binary = main

all: $(binary)

main : main.cpp
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

spv :
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V gl_SubgroupInvocationID.comp.glsl -o gl_SubgroupInvocationID.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupElect.comp.glsl -o subgroupElect.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAll.comp.glsl -o subgroupAll.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAny.comp.glsl -o subgroupAny.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAllEqual.comp.glsl -o subgroupAllEqual.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAdd.comp.glsl -o subgroupAdd.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupMax.comp.glsl -o subgroupMax.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupInclusiveAdd.comp.glsl -o subgroupInclusiveAdd.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupExclusiveAdd.comp.glsl -o subgroupExclusiveAdd.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBroadcast.comp.glsl -o subgroupBroadcast.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBroadcastFirst.comp.glsl -o subgroupBroadcastFirst.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBallot.comp.glsl -o subgroupBallot.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBallotFindLSB.comp.glsl -o subgroupBallotFindLSB.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBallotFindMSB.comp.glsl -o subgroupBallotFindMSB.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupShuffle.comp.glsl -o subgroupShuffle.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupClusteredAdd.comp.glsl -o subgroupClusteredAdd.comp.spv

clean:
	rm -rf $(binary)
	rm -rf *.spv

run:
	LD_LIBRARY_PATH=$(VK_SDK_PATH)/lib VK_LAYER_PATH=$(VK_SDK_PATH)/etc/vulkan/explicit_layer.d \
	VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation ./main
