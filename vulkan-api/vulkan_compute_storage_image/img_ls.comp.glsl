#version 460

#define _X	(800/25)
#define _Y	(800/25)

layout (local_size_x = _X, local_size_y = _Y) in;

layout (set = 0, binding = 0, rgba8) uniform readonly highp image2D srcObj;
layout (set = 0, binding = 1, rgba8) uniform writeonly highp image2D dstObj;

#define MEAN_FILTER_BLOCK_W_SIZE 5
#define MEDIAN_FILTER_BLOCK_W_SIZE 3
#define MEDIAN_TEMPBUF_SIZE ((2*MEDIAN_FILTER_BLOCK_W_SIZE+1)*(2*MEDIAN_FILTER_BLOCK_W_SIZE+1))
#define GAUSSIAN_FILTER_BLOCK_W_SIZE 4
#define GAUSSIAN_FILTER_SIGMA 2
#define PI 3.14159265359

#define BAR_SPACE 160

// check if a location is out of boundary
bool oob(ivec2 loc) {
	if (loc.x < 0 || loc.x >= 800 || loc.y < 0 || loc.y >= 800) {
		return true;
	} else {
		return false;
	}
}

float intensity(in vec4 color) {
	return sqrt((color.x*color.x)+(color.y*color.y)+(color.z*color.z));
}

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	if (pos.x >= 0 && pos.x < BAR_SPACE) {
		imageStore(dstObj, pos, imageLoad(srcObj, pos));
	} else if (pos.x >= BAR_SPACE && pos.x < 2*BAR_SPACE) {
		// apply mean filter
		vec4 kernel = vec4(0u);
		for (int i = -MEAN_FILTER_BLOCK_W_SIZE; i < MEAN_FILTER_BLOCK_W_SIZE; i++) {
			for (int j = -MEAN_FILTER_BLOCK_W_SIZE; j < MEAN_FILTER_BLOCK_W_SIZE; j++) {
				if (!oob(ivec2(i + pos.x, j + pos.y))) {
					kernel += imageLoad(srcObj, ivec2(i + pos.x, j + pos.y));
				}
			}
		}
		kernel /= (MEAN_FILTER_BLOCK_W_SIZE * MEAN_FILTER_BLOCK_W_SIZE);
		imageStore(dstObj, pos, kernel);
	} else if (pos.x >= 2*BAR_SPACE && pos.x < 3*BAR_SPACE) {
		// apply median filter
		// initialize by loading data into 1D array
		vec4 median_tmpbuf[MEDIAN_TEMPBUF_SIZE];
		for (int i = -MEDIAN_FILTER_BLOCK_W_SIZE; i < MEDIAN_FILTER_BLOCK_W_SIZE; i++) {
			for (int j = -MEDIAN_FILTER_BLOCK_W_SIZE; j < MEDIAN_FILTER_BLOCK_W_SIZE; j++) {
				int x = i + pos.x;
				int y = j + pos.y;
				int idx = (i + MEDIAN_FILTER_BLOCK_W_SIZE)*(2*MEDIAN_FILTER_BLOCK_W_SIZE+1) + (j + MEDIAN_FILTER_BLOCK_W_SIZE);
				if (!oob(ivec2(i + pos.x, j + pos.y))) {
					median_tmpbuf[idx] = imageLoad(srcObj, ivec2(x, y));
				} else {
					median_tmpbuf[idx] = vec4(0.0);
				}
			}
		}
		// insertion sort 1D array
		int i, j;
		float key;
		for (i = 1; i < MEDIAN_TEMPBUF_SIZE; i++) {
			key = median_tmpbuf[i].r;
			j = i - 1;
			while(j >= 0 && (median_tmpbuf[j].r > key)) {
				median_tmpbuf[j + 1].r = median_tmpbuf[j].r;
				j--;
			}
			median_tmpbuf[j + 1].r = key;
		}
		for (i = 1; i < MEDIAN_TEMPBUF_SIZE; i++) {
			key = median_tmpbuf[i].g;
			j = i - 1;
			while(j >= 0 && (median_tmpbuf[j].g > key)) {
				median_tmpbuf[j + 1].g = median_tmpbuf[j].g;
				j--;
			}
			median_tmpbuf[j + 1].g = key;
		}
		for (i = 1; i < MEDIAN_TEMPBUF_SIZE; i++) {
			key = median_tmpbuf[i].b;
			j = i - 1;
			while(j >= 0 && (median_tmpbuf[j].b > key)) {
				median_tmpbuf[j + 1].b = median_tmpbuf[j].b;
				j--;
			}
			median_tmpbuf[j + 1].b = key;
		}
		// retrieve the result
		int median_idx = (MEDIAN_FILTER_BLOCK_W_SIZE)*(2*MEDIAN_FILTER_BLOCK_W_SIZE+1) + (MEDIAN_FILTER_BLOCK_W_SIZE);
		imageStore(dstObj, pos, vec4(median_tmpbuf[median_idx]));
	} else if (pos.x >= 3*BAR_SPACE && pos.x < 4*BAR_SPACE) {
		//mat3 sobel_edge_h_op = mat3(vec3(-1.0, -2.0, -1.0), vec3(0.0), vec3(1.0, 2.0, 1.0));
		//mat3 sobel_edge_v_op = mat3(vec3(-1.0, 0.0, -1.0), vec3(2.0, 0.0, -2.0), vec3(1.0, 0.0, -1.0));
		mat3 sobel_tmpbuf; // store intensity
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				int x = i + pos.x;
				int y = j + pos.y;
				if (!oob(ivec2(i + pos.x, j + pos.y))) {
					sobel_tmpbuf[i+1][j+1] = intensity(imageLoad(srcObj, ivec2(x, y)));
				} else {
					sobel_tmpbuf[i+1][j+1] = 0.0;
				}
			}
		}
		float sobel_dy = sobel_tmpbuf[2][0] + sobel_tmpbuf[2][1]*2 + sobel_tmpbuf[2][2]
			- (sobel_tmpbuf[0][0] + sobel_tmpbuf[0][1]*2 + sobel_tmpbuf[0][2]);
		float sobel_dx = sobel_tmpbuf[0][2] + sobel_tmpbuf[1][2]*2 + sobel_tmpbuf[2][2]
			- (sobel_tmpbuf[0][0] + sobel_tmpbuf[1][0]*2 + sobel_tmpbuf[2][0]);
		vec4 sobel = vec4(sqrt(sobel_dx*sobel_dx + sobel_dy*sobel_dy));
		imageStore(dstObj, pos, sobel);
	} else if (pos.x >= 4*BAR_SPACE && pos.x < 5*BAR_SPACE) {
		// apply gaussian blur
		vec4 pixel = vec4(0.0);
		for (int dx = -GAUSSIAN_FILTER_BLOCK_W_SIZE; dx <= GAUSSIAN_FILTER_BLOCK_W_SIZE; dx++) {
			for (int dy = -GAUSSIAN_FILTER_BLOCK_W_SIZE; dy <= GAUSSIAN_FILTER_BLOCK_W_SIZE; dy++) {
				int x = pos.x + dx;
				int y = pos.y + dy;
				if (!oob(ivec2(x, y))) {
					float c = 0.5 / (PI*GAUSSIAN_FILTER_SIGMA*GAUSSIAN_FILTER_SIGMA) * exp(-float(dx*dx + dy*dy) / (2.0 * GAUSSIAN_FILTER_SIGMA * GAUSSIAN_FILTER_SIGMA));
					pixel += imageLoad(srcObj, ivec2(x, y)) * c;
				}
			}
		}
		imageStore(dstObj, pos, pixel);
	}
}
