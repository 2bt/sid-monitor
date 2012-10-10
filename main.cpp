#include <stdio.h>
#include <math.h>
#include <SDL/SDL.h>
#include "resid-0.16/sid.h"


extern unsigned char memory[65536];
unsigned short c64SidLoad(const char* filename, unsigned short* init_addr,
	unsigned short* play_addr, unsigned char* sub_song_start,
	unsigned char* max_sub_songs, unsigned char* speed, char* name,
	char* author, char* copyright);
void cpuJSR(unsigned short npc, unsigned char na);
void c64Init(void);


enum {
	MIXRATE = 44100,
	FRAMES_PER_SECOND = 50,
	RECORD_LENGTH = FRAMES_PER_SECOND * 60 * 10,
	FRAME_LENGTH = MIXRATE / FRAMES_PER_SECOND,
};
unsigned char record[RECORD_LENGTH][25];

bool fill_record(const char* filename, int number) {

	unsigned short init_addr;
	unsigned short play_addr;
	unsigned char song;
	unsigned char max_songs;
	unsigned char speed;

	char name[32];
	char author[32];
	char copyright[32];

	c64Init();
	int ret = c64SidLoad(filename, &init_addr, &play_addr, &song, &max_songs,
		&speed, name, author, copyright);
	if (!ret) return false;

	if (number > 0 && number <= max_songs + 1) song = number - 1;

	cpuJSR(init_addr, song);
	printf("%s - %s - %s - %d/%d\n", name, author, copyright, song + 1, max_songs + 1);


	for (int i = 0; i < RECORD_LENGTH; i++) {
		cpuJSR(play_addr, 0);
		memcpy(&record[i][0], &memory[0xd400], 25);
	}
	return true;
}


SID sid;
int voice_flags[3] = { 1, 1, 1 };
volatile int record_pos = 0;
volatile int frame_pos = 0;


void audio_callback(void* userdata, unsigned char* stream, int len) {
	short* buffer = (short*) stream;
	for (int i = 0; i < len>>1; i++) {
		sid.clock(17734472 / (18 * MIXRATE));	// PAL
		if (++frame_pos == FRAME_LENGTH) {
			frame_pos = 0;
			for (int c = 0; c < 3; c++) {
				for (int r = 0; r < 7; r++) {
					int a = voice_flags[c] ? record[record_pos][c * 7 + r] : 0;
					sid.write(c * 7 + r, a);
				}
			}
			for (int r = 21; r < 25; r++) sid.write(r, record[record_pos][r]);
			if (++record_pos == RECORD_LENGTH) record_pos = 0;
		}
		buffer[i] = sid.output();
	}
}


bool start_audio() {
	sid.reset();
	sid.set_chip_model(MOS6581);
	sid.enable_filter(true);
	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		1, 0, 1024, 0, 0, &audio_callback, NULL
	};
	return !SDL_OpenAudio(&spec, &spec);
}



enum { WIDTH = 800, HEIGHT = 600 };
SDL_Surface* screen;
unsigned int* pixels;
int zoom = 3;
int bar_length = 8 * 6;
int bar_offset = 2;



inline unsigned int get_pixel(int x, int y) {
	if (x >= 0 && x < WIDTH && y > 0 && y <= HEIGHT)
		return pixels[x + y * WIDTH];
	return 0;
}
inline void set_pixel(int x, int y, unsigned int color) {
	if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
		pixels[x + y * WIDTH] = color;
}


#define COLOR_R(x)			(((x) >> 16) & 0xff)
#define COLOR_G(x)			(((x) >> 8) & 0xff)
#define COLOR_B(x)			(((x) >> 0) & 0xff)
#define COLOR_RGB(r, g, b)	(((r) << 16) | ((g) << 8) | (b))


void draw() {
	// clear
	SDL_FillRect(screen, NULL, 0x000000);

	int cursor = (record_pos * FRAME_LENGTH + frame_pos) * zoom / FRAME_LENGTH;
	int offset = 0;
	if (cursor > WIDTH / 2) {
		offset = cursor - WIDTH / 2;
		int max = (RECORD_LENGTH * FRAME_LENGTH) * zoom / FRAME_LENGTH - WIDTH;
		if (offset > max) offset = max;
	}


	for (int x = 0; x < WIDTH; x++) {

		unsigned int color = 0;

		// time bar
		if ((x + offset - zoom * bar_offset) % (zoom * bar_length) == 0) color = 0x101010;
		if ((x + offset - zoom * bar_offset) % (zoom * bar_length * 4) == 0) color = 0x303030;

		// cursor
		if (x + offset == cursor) color = 0x606060;
		if (color) for (int y = 0; y < HEIGHT; y++) set_pixel(x, y, color);


		for (int c = 0; c < 3; c++) {
			if (!voice_flags[c]) continue;
			unsigned char* regs = &record[(x + offset) / zoom][c * 7];

			int gate = regs[4] & 1;
			int noise = (regs[4] >> 7) & 1;

			double freq = regs[0] + regs[1] * 256;

			freq /= 100;
			double note = 12.0 * log2(freq * 17734472.0 / (18.0 * 16.35 * 16777216.0 ));
			int n = (note + 60) * 6.0 + 100;

			color = 0xff << (18 - c * 8);
			if (noise) color |= 0x666666;


			color = COLOR_RGB(
				COLOR_R(color) * (1 + gate * 3) / 4,
				COLOR_G(color) * (1 + gate * 3) / 4,
				COLOR_B(color) * (1 + gate * 3) / 4
			);


			set_pixel(x, HEIGHT - n - 2, get_pixel(x, HEIGHT - n - 1) | color);
			set_pixel(x, HEIGHT - n - 1, get_pixel(x, HEIGHT - n - 1) | color);
			set_pixel(x, HEIGHT - n - 0, get_pixel(x, HEIGHT - n - 1) | color);


		}

	}
}



int main(int argc, char** argv) {

	if (argc < 2 || argc > 3) {
		printf("usage: %s sidfile [number]\n", argv[0]);
		return 0;
	}

	fill_record(argv[1], argc == 3 ? atoi(argv[2]) : 0);
	start_audio();


	if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
	screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (!screen) {
		SDL_Quit();
		return 1;
	}
	unsigned char* keys = SDL_GetKeyState(NULL);
	pixels = (unsigned int*) screen->pixels;


	bool playing = false;
	bool running = true;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					running = false;
					break;

				case SDLK_SPACE:
					playing ^= 1;
					SDL_PauseAudio(!playing);
					break;
				case SDLK_BACKSPACE:
					playing = 0;
					SDL_PauseAudio(!playing);
					record_pos = 0;
					frame_pos = 0;
					break;

				case SDLK_1:
					voice_flags[0] ^= 1;
					break;
				case SDLK_2:
					voice_flags[1] ^= 1;
					break;
				case SDLK_3:
					voice_flags[2] ^= 1;
					break;

				case SDLK_PLUS:
					zoom++;
					break;
				case SDLK_MINUS:
					if (zoom > 1) zoom--;
					break;

				case SDLK_d:
					bar_offset++;
					break;
				case SDLK_a:
					bar_offset--;
					break;
				case SDLK_w:
					bar_length++;
					break;
				case SDLK_s:
					bar_length--;
					break;




				default: break;
				}
			}
		}


		int shift = keys[SDLK_LSHIFT] | keys[SDLK_RSHIFT];
		if (keys[SDLK_RIGHT]) {
			record_pos += 1 + shift * 10;
			if (!playing) frame_pos = 0;
			if (record_pos >= RECORD_LENGTH) record_pos -= RECORD_LENGTH;
		}
		if (keys[SDLK_LEFT]) {
			record_pos -= 1 + shift * 10;
			if (!playing) frame_pos = 0;
			if (record_pos < 0) record_pos += RECORD_LENGTH;
		}


		SDL_LockSurface(screen);
		draw();
		SDL_UnlockSurface(screen);
		SDL_Flip(screen);
		SDL_Delay(10);
	}

	SDL_Quit();
	return 0;
}
