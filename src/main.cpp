#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "resid-0.16/sid.h"
#include "record.h"



Record record;


SID sid;
int voice_flags[3] = { 1, 1, 1 };
volatile int record_pos = 0;
volatile int frame_pos = 0;
unsigned char* keys;
bool playing = false;


void audio_callback(void* userdata, unsigned char* stream, int len) {

	int ctrl = keys[SDLK_RCTRL] | keys[SDLK_LCTRL];
	int x = keys[SDLK_RIGHT] - keys[SDLK_LEFT];
	x *= 1 + (keys[SDLK_LSHIFT] | keys[SDLK_RSHIFT]) * 5;

	short* buffer = (short*) stream;
	for (int i = 0; i < len >> 1; i++) {

		if (playing) {
			sid.clock(17734472 / (18 * MIXRATE)); // PAL
			buffer[i] = sid.output();
		}
		else buffer[i] = 0;

		if (!ctrl) frame_pos += x > 0 ? x : x < 0 ? x : playing;

		int d = (frame_pos >= FRAME_LENGTH) - (frame_pos < 0);
		if (d || ctrl) {
			frame_pos = (frame_pos + FRAME_LENGTH) % FRAME_LENGTH;
			for (int c = 0; c < 3; c++) {
				for (int r = 0; r < 7; r++) {
					int a = record.get_reg(record_pos, c * 7 + r);
					if (r >= 5 && !voice_flags[c]) a = 0;
					sid.write(c * 7 + r, a);
				}
			}
			for (int r = 21; r < 25; r++) sid.write(r, record.get_reg(record_pos, r));

			record_pos += d;
			if (record_pos >= RECORD_LENGTH) record_pos -= RECORD_LENGTH;
			if (record_pos < 0) record_pos += RECORD_LENGTH;
		}
	}
}

bool filter_enabled = true;

void start_audio() {
	sid.reset();
//	sid.set_chip_model(MOS6581);
	sid.set_chip_model(MOS8580);
	sid.enable_filter(filter_enabled);
	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		1, 0, 1024 / 2, 0, 0, &audio_callback, NULL
	};
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);
}


SDL_Surface* screen;
SDL_Surface* font_img;
int a_freq = 440;
int zoom = 3;
int vert_zoom = 10;
int vert_pos = -12;
int bar_length = 8 * 6;
int bar_offset = 2;
bool show_filter = false;
bool show_bars = true;


inline unsigned int get_pixel(int x, int y) {
	if (x >= 0 && x < screen->w && y >= 0 && y < screen->h)
		return ((unsigned int*)screen->pixels)[x + y * screen->pitch / 4];
	return 0;
}
inline void set_pixel(int x, int y, unsigned int color) {
	if (x >= 0 && x < screen->w && y >= 0 && y < screen->h)
		((unsigned int*)screen->pixels)[x + y * screen->pitch / 4] = color;
}
void print(short x, short y, const char* text, ...) {
	char line[256];
	va_list args;
	va_start(args, text);
	vsnprintf(line, 256, text, args);
	va_end(args);

	SDL_Rect src = { 0, 0, 8, 8 };
	SDL_Rect dst = { x, y, 0, 0 };
	unsigned char c;
	int i = 0;
	while ((c = (unsigned char)line[i++])) {
		src.x = c % 8 * 8;
		src.y = c / 8 * 8;
		SDL_BlitSurface(font_img, &src, screen, &dst);
		dst.x += 8;
	}
}


#define COLOR_R(x)			(((x) >> 16) & 0xff)
#define COLOR_G(x)			(((x) >> 8) & 0xff)
#define COLOR_B(x)			(((x) >> 0) & 0xff)
#define COLOR_RGB(r, g, b)	(((r) << 16) | ((g) << 8) | (b))


void draw() {
	SDL_FillRect(screen, NULL, 0x000000);
	SDL_Rect rect = { 0, 0, (Uint16) screen->w, 0 };

	for (int n = 0; n < screen->h / vert_zoom + 2; n++) {
		int no = n - screen->h / vert_zoom / 2;
		int y = screen->h / 2 - vert_zoom / 2 + no * vert_zoom;
		rect.y = y;
		rect.h = vert_zoom - 1;
		int note = (vert_pos - no + 1200 - 3);

		static const unsigned int key_colors[] = {
			0x222222, 0x111111, 0x333333
		};
		SDL_FillRect(screen, &rect, key_colors["210100101010"[note%12] - '0']);

	}


	SDL_LockSurface(screen);

	int cursor = (record_pos * FRAME_LENGTH + frame_pos) * zoom / FRAME_LENGTH;
	int offset = 0;
	if (cursor > screen->w / 2) {
		offset = cursor - screen->w / 2;
		int max = (RECORD_LENGTH * FRAME_LENGTH) * zoom / FRAME_LENGTH - screen->w;
		if (offset > max) offset = max;
	}
	int cursor_note[3];
	for (int x = 0; x < screen->w; x++) {

		if (show_bars) {
			if ((x + offset - zoom * bar_offset) % (zoom * bar_length) == 0)
				for (int y = 0; y < screen->h; y++) set_pixel(x, y, 0x444444);
		}

		int pos = (x + offset) / zoom;
		for (int c = 0; c < 3; c++) {
			if (!voice_flags[c]) continue;

			int gate  = record.get_reg(pos, c * 7 + 4) & 1;
			int noise = record.get_reg(pos, c * 7 + 4) / 128;
			int freq  = record.get_reg(pos, c * 7 + 0)
			          + record.get_reg(pos, c * 7 + 1) * 256;

			double real_freq = freq * 17734472.0 / (18 << 24);
			double note = log2(real_freq / (double) a_freq) * 12.0;
			if (x + offset == cursor) {
				cursor_note[c] = (int) (note + 57.5);
			}


			int y = screen->h / 2 - (note - vert_pos) * vert_zoom;

			unsigned int color = 0xff << (18 - c * 8);
			if (noise) color |= 0x666666;

			color = COLOR_RGB(
				COLOR_R(color) * (1 + gate * 2) / 3,
				COLOR_G(color) * (1 + gate * 2) / 3,
				COLOR_B(color) * (1 + gate * 2) / 3
			);

			set_pixel(x, y + 2, get_pixel(x, y + 2) | color);
			set_pixel(x, y + 1, get_pixel(x, y + 1) | color);
			set_pixel(x, y    , get_pixel(x, y    ) | color);
			set_pixel(x, y - 1, get_pixel(x, y - 1) | color);
			set_pixel(x, y - 2, get_pixel(x, y - 2) | color);

		}

		if (show_filter) {
			int filter_freq = record.get_reg(pos, 22);
			for (int y = screen->h - 1 - filter_freq; y < screen->h; y++)
				set_pixel(x, y, get_pixel(x, y) | 0x0f0f00);
		}

		// cursor
		if (x + offset == cursor)
			for (int y = 0; y < screen->h; y++) set_pixel(x, y, 0x888888);
	}
	SDL_UnlockSurface(screen);

	// print stuff
	print(8, 8, "%s", record.get_title());
	print(screen->w - 8 * 20,  8, "       time:  %02d:%02d",
		record_pos / FRAMES_PER_SECOND / 60,
		record_pos / FRAMES_PER_SECOND % 60);
	print(screen->w - 8 * 20, 24, "   position: %6d", record_pos);
	print(screen->w - 8 * 20, 40, " bar length: %6d", bar_length);
	print(screen->w - 8 * 20, 56, "     tuning: %6d", a_freq);

	for (int c = 0; c < 3; c++) {
		if (!voice_flags[c]) continue;

		for (int r = 0; r < 7; r++) {
			print(8 + r * 24, 24 + c * 16, "%02X", record.get_reg(record_pos, c * 7 + r));
		}


		int control = record.get_reg(record_pos, c * 7 + 4);
		print(200 - 8 * 2, 24 + c * 16, "%c%c%c%c",
			".G"[!!(control & 1)],
			".S"[!!(control & 2)],
			".R"[!!(control & 4)],
			".T"[!!(control & 8)]);

		int n = cursor_note[c];
		if (n > 0) {
			print(232, 24 + c * 16, "%c%c%d %d",
				"CCDDEFFGGAAB"[n % 12],
				"-#-#--#-#-#-"[n % 12],
				n / 12,
				n);
		}


	}
	for (int r = 0; r < 4; r++) {
		print(8 + r * 24, 72, "%02X", record.get_reg(record_pos, 21 + r));
	}

}


int main(int argc, char** argv) {

	if (argc < 2 || argc > 3) {
		printf("usage:\n");
		printf(" %s sidfile [number]\n", argv[0]);
		printf(" %s logfile\n", argv[0]);
		return 0;
	}

	if (!record.load(argv[1], argc == 3 ? atoi(argv[2]) : 0)) {
		printf("read error\n");
		return 1;
	}


	if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
	screen = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
	if (!screen) {
		SDL_Quit();
		return 1;
	}
	keys = SDL_GetKeyState(NULL);
	font_img = IMG_Load("font.png");

	start_audio();

	bool running = true;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;

			case SDL_VIDEORESIZE:
				screen = SDL_SetVideoMode(event.resize.w, event.resize.h,
					32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
				break;

			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_TAB:
					SDL_EnableKeyRepeat(0, 0);
					break;
				default: break;
				}
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					running = false;
					break;

				case SDLK_TAB:
					SDL_EnableKeyRepeat(10, 10);
					break;

				case SDLK_LEFT:
				case SDLK_RIGHT:
					if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
						record_pos += event.key.keysym.sym == SDLK_RIGHT;
						record_pos -= event.key.keysym.sym == SDLK_LEFT;
						frame_pos = 0;
					}
					break;

				case SDLK_SPACE:
					playing ^= 1;
					break;
				case SDLK_BACKSPACE:
					playing = 0;
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


				case SDLK_y:
					filter_enabled ^= 1;
					sid.enable_filter(filter_enabled);
					break;

				case SDLK_f:
					show_filter ^= 1;
					break;
				case SDLK_t:
					show_bars ^= 1;
					break;

				case SDLK_PLUS:
					zoom++;
					break;
				case SDLK_MINUS:
					if (zoom > 1) zoom--;
					break;

				case SDLK_PAGEUP:
					vert_zoom++;
					break;
				case SDLK_PAGEDOWN:
					if (vert_zoom > 1) vert_zoom--;
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

				case SDLK_j:
					a_freq++;
					break;
				case SDLK_h:
					a_freq--;
					break;

				default: break;
				}
			}
		}

		vert_pos += keys[SDLK_UP] - keys[SDLK_DOWN];

		draw();
		SDL_Flip(screen);
		SDL_Delay(10);
	}

	SDL_FreeSurface(font_img);
	SDL_Quit();
	return 0;
}
