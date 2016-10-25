#include <cstring>
#include <cstdio>
#include "record.h"


extern unsigned char memory[65536];
unsigned short c64SidLoad(const char* filename, unsigned short* init_addr,
	unsigned short* play_addr, unsigned char* sub_song_start,
	unsigned char* max_sub_songs, unsigned char* speed, char* name,
	char* author, char* copyright);
void cpuJSR(unsigned short npc, unsigned char na);
void c64Init(void);



bool Record::load(const char* filename, int number) {

	// check suffix
	const char* dot = strrchr(filename, '.');
	if (dot && strcmp(dot, ".txt") == 0) {

		// load log file

		snprintf(title, sizeof(title), "%s\n", filename);

		FILE* f = fopen(filename, "r");
		if (!f) return false;

		int n = 1;

		unsigned int dt, addr, val;
		while (fscanf(f, "%d [%d] = %x ", &dt, &addr, &val) == 3) {

			int idle = 0;
			while (idle++ < 10 && dt > 17000) {
				dt -= 17000;
				if (++n >= RECORD_LENGTH) goto END;
				memcpy(&data[n], &data[n - 1], 25);
			}
			data[n][addr] = val;
		}
END:
		fclose(f);
		return true;
	}


	// load sid tune

	unsigned short init_addr;
	unsigned short play_addr;
	unsigned char speed;
	unsigned char song;
	unsigned char max_songs;
	char name[32];
	char author[32];
	char copyright[32];

	c64Init();
	int ret = c64SidLoad(filename, &init_addr, &play_addr, &song, &max_songs,
		&speed, name, author, copyright);
	if (!ret) return false;

	if (number > 0 && number <= max_songs + 1) song = number - 1;

	cpuJSR(init_addr, song);
	snprintf(title, sizeof(title), "%s - %s - %s - %d/%d\n",
		name, author, copyright, song + 1, max_songs + 1);

	for (int i = 0; i < RECORD_LENGTH; i++) {
		cpuJSR(play_addr, 0);
		memcpy(&data[i][0], &memory[0xd400], 25);
	}
	return true;
}

