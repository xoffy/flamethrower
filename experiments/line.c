
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../src/picture.h"
#include "../src/secamizer.h"
#include "../src/noise.h"
#include "../src/util.h"

void generate_random_line(YCCPicture *pic) {
	noise_amplitude = 256.0;
	noise_scale = 0.024;

	int halfheight = pic->height / 2;

	for (int x = 0; x < pic->width; x++) {
		for (int y = 0; y < halfheight; y++) {
			pic->luma[y * pic->width + x] = noise(x);
		}
	}

	noise_amplitude = 256.0;
	noise_scale = 0.048;

	int cwidth = pic->width / 4;
	int cheight = pic->height / 2 / 2;

	for (int cx = 0; cx < cwidth; cx++) {
		for (int cy = 0; cy < cheight; cy++) {
			pic->cb[cy * cwidth + cx] = COLOR_CLAMP(noise(cx));
			pic->cr[cy * cwidth + cx] = COLOR_CLAMP(noise(cx + 1048576));
		}
	}
}

void generate_color_bars(YCCPicture *pic) {
	int halfheight = pic->height / 2;

	for (int y = 0; y < halfheight; y++) {
		for (int x = 0; x < pic->width; x++) {
			int luma = COLOR_CLAMP(floor((pic->width - x) / (pic->width / 8)) * 32);
			pic->luma[y * pic->width + x] = luma;
		}
	}

	int cwidth = pic->width / 4;
	int cheight = pic->height / 2;
	int colors[8][2] = {
		{ 128, 128 },	// gray
		{ 64, 128 },	// yellow
		{ 255, 0 },		// cyan
		{ 0, 0 },		// green
		{ 255, 255 },	// magenta
		{ 64, 255 },	// red
		{ 255, 64 },	// blue
		{ 128, 128 }	// black
	};

	for (int cx = 0; cx < cwidth; cx++) {
		for (int cy = 0; cy < (cheight / 2); cy++) {
			int idx = floor(cx / (cwidth / 8));
			pic->cb[cy * cwidth + cx] = colors[idx][0];
			pic->cr[cy * cwidth + cx] = colors[idx][1];
		}
	}
}

void generate_black_picture(YCCPicture *pic) {
	int halfheight = pic->height / 2;

	for (int x = 0; x < pic->width; x++) {
		for (int y = 0; y < halfheight; y++) {
			pic->luma[y * pic->width + x] = LUMA_CLAMP(0);
		}
	}
}

void luma_noise_scan2(YCCPicture *frame, double amp, int x, int y) {
	int luma_idx = (y * frame->width) + x;
	// int random_offset = rand() % 8388608;

	frame->luma[luma_idx] = COLOR_CLAMP(frame->luma[luma_idx]
		+ noise2(x + 8388608, amp, 0.192) - (amp * 0.5));
}

void chroma_noise_scan2(YCCPicture *frame, double amp, int cx, int cy) {
	int chroma_idx = (cy * (frame->width / 4)) + cx;

	double addnoise = noise2(cx, 256.0, 0.14);

	double cbnoise = noise2(cx + 16384 * cy, amp, 0.97) - (amp * 0.5);
	double crnoise = noise2(cx + 32768 * cy, amp, 0.97) - (amp * 0.5);

	frame->cb[chroma_idx] = COLOR_CLAMP(frame->cb[chroma_idx] + cbnoise);
	frame->cr[chroma_idx] = COLOR_CLAMP(frame->cr[chroma_idx] + crnoise);
}

void secamizer_scan2(Secamizer *self, YCCPicture *frame, int cx, int cy) {
	int luma_idx = ((cy * 2) * frame->width) + (cx * 4);
	int luma = (frame->luma[luma_idx + 0] + frame->luma[luma_idx + 1]
		+ frame->luma[luma_idx + 2] + frame->luma[luma_idx + 3]) / 4;

	uint8_t *chroma;
	int chroma_idx = (cy * (frame->width / 4)) + cx;

	if (cy % 2 == 0) {
		chroma = frame->cr + chroma_idx;
	} else {
		chroma = frame->cb + chroma_idx;
	}

	static uint8_t last_chroma_0 = 128;
	static uint8_t last_chroma_1 = 128;

	if (last_chroma_1 - last_chroma_0 != 0) {
		noise_amplitude = 0.2;
		noise_scale = 0.192;
		last_chroma_0 = *chroma;
		last_chroma_1 = *chroma =
			COLOR_CLAMP((last_chroma_1 - *chroma)
			* (0.75 + noise(cx + cy) - 0.1) + *chroma);
		return;
	}

	last_chroma_0 = *chroma;

	noise_amplitude = 32;
	noise_scale = 0.192;
	int shift = noise(cx + cy + 7240) - 16;

	if (abs(luma - *chroma) > 128) {
		last_chroma_1 = *chroma = 192;
	} else {
		last_chroma_1 = *chroma;
	}
}

void secam_test(YCCPicture *pic) {
	for (int x = 0; x < pic->width; x++) {
		for (int y = (pic->height / 4); y < (pic->height / 2); y++) {
			; // luma_noise_scan2(pic, 4.0, x, y);
		}
	}

	Secamizer *secamizer = calloc(1, sizeof(Secamizer));
	secamizer->thrshld = 0.024;
	secamizer->rndm = 0.0;

	int cwidth = pic->width / 4;
	int cheight = pic->height / 2;

	for (int cx = 1; cx < cwidth; cx++) {
		for (int cy = (cheight / 4); cy < (cheight / 2); cy++) {
			chroma_noise_scan2(pic, 36.0, cx, cy);
			// secamizer_scan2(secamizer, pic, cx, cy);
		}
	}

	free(secamizer);
}

void make_graph(YCCPicture *pic, int y) {
	// luma
	for (int x = 0; x < pic->width; x++) {
		pic->luma[(pic->height / 4 * 3) * pic->width + x] = 144;

		int luma = pic->luma[y * pic->width + x];
		pic->luma[(pic->height - luma - 1) * pic->width + x] = 0;
	}

	// chroma
	int cwidth = pic->width / 4;
	int cheight = pic->height / 2;
	int cy = y / 2;
	for (int cx = 0; cx < cwidth; cx++) {
		int blue = pic->cb[cy * cwidth + cx];
		int red = pic->cr[cy * cwidth + cx];
		pic->cb[(cheight - blue / 2 - 1) * cwidth + cx] = 255;
		pic->cr[(cheight - red / 2 - 1) * cwidth + cx] = 255;
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "This executable takes exactly 1 argument.\n");
		return 1;
	}

	srand(0);
	noise_init();

	YCCPicture *pic = ycc_new(1024, 512);
	ycc_reset(pic);

	// generate_random_line(pic);
	generate_color_bars(pic);
	// generate_black_picture(pic);
	secam_test(pic);
	make_graph(pic, 192);

	ycc_save_picture(pic, argv[1], NULL);
	ycc_delete(&pic);

	return 0;
}

