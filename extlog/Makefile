VK_SDK_PATH = /home/yuewu01/v/1.3.204.1/x86_64

.PHONY: clean run all

bin = es_main gl_main
src = main.cpp

ES_CXXFLAGS = -std=c++11 -I. -I$(VK_SDK_PATH)/include -Wall -Wformat -lglfw -lGLESv2 -L$(VK_SDK_PATH)/lib -lvulkan -DES_BACKEND=1
GL_CXXFLAGS = -std=c++11 -I. -I$(VK_SDK_PATH)/include -Wall -Wformat -lglfw -lGL -lGLEW -L$(VK_SDK_PATH)/lib -lvulkan -DGL_BACKEND=1

all:
	$(CXX) $(ES_CXXFLAGS) -c -o es_main.o main.cpp && $(CXX) -o es_main es_main.o $(ES_CXXFLAGS) \
	&& $(CXX) $(GL_CXXFLAGS) -c -o gl_main.o main.cpp && $(CXX) -o gl_main gl_main.o $(GL_CXXFLAGS)

clean:
	rm -rf $(bin) *.o

run:
	LD_LIBRARY_PATH=$(VK_SDK_PATH)/lib VK_LAYER_PATH=$(VK_SDK_PATH)/etc/vulkan/explicit_layer.d \
	VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation ./gl_main