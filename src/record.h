#pragma once

enum {
	MIXRATE = 44100,
	FRAMES_PER_SECOND = 50,
	FRAME_LENGTH = MIXRATE / FRAMES_PER_SECOND,
	RECORD_LENGTH = FRAMES_PER_SECOND * 60 * 10,
};


class Record {
public:

	bool load(const char* filename, int number=0);
	unsigned int get_reg(int pos, int reg_nr) const {
		return data[pos][reg_nr];
	}

	const char* get_title() const {
		return title;
	}

private:
	char title[128];
	unsigned char data[RECORD_LENGTH][25];
};
