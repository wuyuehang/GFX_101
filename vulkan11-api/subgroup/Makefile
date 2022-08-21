VK_SDK_PATH = /home/yuewu01/v/1.3.216.0/x86_64

common = ../../common/vulkan
thirdparty = ../../thirdparty

CXXFLAGS = -std=c++11 -I$(VK_SDK_PATH)/include -I$(common) -I$(thirdparty) -Wall -Wformat

LDFLAGS = -L$(VK_SDK_PATH)/lib -lvulkan

.PHONY: clean run all

binary = main

src = main.cpp
src += $(common)/VulkanCore.cpp \
	$(common)/VulkanCommon.cpp

obj += $(addsuffix .o, $(basename $(notdir $(src))))

all: $(binary)

$(binary) : $(obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(common)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

spv :
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V gl_SubgroupInvocationID.comp -o gl_SubgroupInvocationID.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupElect.comp -o subgroupElect.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAll.comp -o subgroupAll.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAny.comp -o subgroupAny.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAllEqual.comp -o subgroupAllEqual.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupAdd.comp -o subgroupAdd.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupMax.comp -o subgroupMax.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupInclusiveAdd.comp -o subgroupInclusiveAdd.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupExclusiveAdd.comp -o subgroupExclusiveAdd.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBroadcast.comp -o subgroupBroadcast.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBroadcastFirst.comp -o subgroupBroadcastFirst.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBallot.comp -o subgroupBallot.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBallotFindLSB.comp -o subgroupBallotFindLSB.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupBallotFindMSB.comp -o subgroupBallotFindMSB.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupShuffle.comp -o subgroupShuffle.comp.spv
	$(VK_SDK_PATH)/bin/glslangValidator --target-env spirv1.5 -V subgroupClusteredAdd.comp -o subgroupClusteredAdd.comp.spv

clean:
	rm -rf $(binary) $(obj) *.spv

run:
	LD_LIBRARY_PATH=$(VK_SDK_PATH)/lib VK_LAYER_PATH=$(VK_SDK_PATH)/etc/vulkan/explicit_layer.d \
	VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation ./main
