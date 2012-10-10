/*
	This code is stolen from here (I think):
	http://www.rsinsch.de/?id=download&s=k2&action=showall&category=77021f9669&lang=en
*/
#include <stdio.h>
#include <string.h>

#define FLAG_N 128
#define FLAG_V 64
#define FLAG_B 16
#define FLAG_D 8
#define FLAG_I 4
#define FLAG_Z 2
#define FLAG_C 1


enum {
	adc, and_, asl, bcc, bcs, beq, bit, bmi, bne, bpl, brk, bvc, bvs, clc,
	cld, cli, clv, cmp, cpx, cpy, dec, dex, dey, eor, inc, inx, iny, jmp,
	jsr, lda, ldx, ldy, lsr, nop, ora, pha, php, pla, plp, rol, ror, rti,
	rts, sbc, sec, sed, sei, sta, stx, sty, tax, tay, tsx, txa, txs, tya,
	xxx
};

unsigned char memory[65536];

unsigned char getmem(unsigned short addr) {
	if(addr == 0xdd0d) memory[addr] = 0;
	return memory[addr];
}

void setmem(unsigned short addr, unsigned char value) {
	memory[addr] = value;
	if((addr & 0xff00) == 0xd400) {
//		printf("%02d %02X\n", addr & 0x1f, value);
	}
}

//enum { imp, imm, abs, absx, absy, zp, zpx, zpy, ind, indx, indy, acc, rel};
#define imp 0
#define imm 1
#define abs 2
#define absx 3
#define absy 4
#define zp 6
#define zpx 7
#define zpy 8
#define ind 9
#define indx 10
#define indy 11
#define acc 12
#define rel 13

static int opcodes[256] = {
	brk,ora,xxx,xxx,xxx,ora,asl,xxx,php,ora,asl,xxx,xxx,ora,asl,xxx,
	bpl,ora,xxx,xxx,xxx,ora,asl,xxx,clc,ora,xxx,xxx,xxx,ora,asl,xxx,
	jsr,and_,xxx,xxx,bit,and_,rol,xxx,plp,and_,rol,xxx,bit,and_,rol,xxx,
	bmi,and_,xxx,xxx,xxx,and_,rol,xxx,sec,and_,xxx,xxx,xxx,and_,rol,xxx,
	rti,eor,xxx,xxx,xxx,eor,lsr,xxx,pha,eor,lsr,xxx,jmp,eor,lsr,xxx,
	bvc,eor,xxx,xxx,xxx,eor,lsr,xxx,cli,eor,xxx,xxx,xxx,eor,lsr,xxx,
	rts,adc,xxx,xxx,xxx,adc,ror,xxx,pla,adc,ror,xxx,jmp,adc,ror,xxx,
	bvs,adc,xxx,xxx,xxx,adc,ror,xxx,sei,adc,xxx,xxx,xxx,adc,ror,xxx,
	xxx,sta,xxx,xxx,sty,sta,stx,xxx,dey,xxx,txa,xxx,sty,sta,stx,xxx,
	bcc,sta,xxx,xxx,sty,sta,stx,xxx,tya,sta,txs,xxx,xxx,sta,xxx,xxx,
	ldy,lda,ldx,xxx,ldy,lda,ldx,xxx,tay,lda,tax,xxx,ldy,lda,ldx,xxx,
	bcs,lda,xxx,xxx,ldy,lda,ldx,xxx,clv,lda,tsx,xxx,ldy,lda,ldx,xxx,
	cpy,cmp,xxx,xxx,cpy,cmp,dec,xxx,iny,cmp,dex,xxx,cpy,cmp,dec,xxx,
	bne,cmp,xxx,xxx,xxx,cmp,dec,xxx,cld,cmp,xxx,xxx,xxx,cmp,dec,xxx,
	cpx,sbc,xxx,xxx,cpx,sbc,inc,xxx,inx,sbc,nop,xxx,cpx,sbc,inc,xxx,
	beq,sbc,xxx,xxx,xxx,sbc,inc,xxx,sed,sbc,xxx,xxx,xxx,sbc,inc,xxx
};

static int modes[256] = {
	imp,indx,xxx,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,abs,abs,abs,xxx,
	rel,indy,xxx,xxx,xxx,zpx,zpx,xxx,imp,absy,xxx,xxx,xxx,absx,absx,xxx,
	abs,indx,xxx,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,abs,abs,abs,xxx,
	rel,indy,xxx,xxx,xxx,zpx,zpx,xxx,imp,absy,xxx,xxx,xxx,absx,absx,xxx,
	imp,indx,xxx,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,abs,abs,abs,xxx,
	rel,indy,xxx,xxx,xxx,zpx,zpx,xxx,imp,absy,xxx,xxx,xxx,absx,absx,xxx,
	imp,indx,xxx,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,ind,abs,abs,xxx,
	rel,indy,xxx,xxx,xxx,zpx,zpx,xxx,imp,absy,xxx,xxx,xxx,absx,absx,xxx,
	imm,indx,xxx,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,abs,abs,abs,xxx,
	rel,indy,xxx,xxx,zpx,zpx,zpy,xxx,imp,absy,acc,xxx,xxx,absx,absx,xxx,
	imm,indx,imm,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,abs,abs,abs,xxx,
	rel,indy,xxx,xxx,zpx,zpx,zpy,xxx,imp,absy,acc,xxx,absx,absx,absy,xxx,
	imm,indx,xxx,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,abs,abs,abs,xxx,
	rel,indy,xxx,xxx,zpx,zpx,zpx,xxx,imp,absy,acc,xxx,xxx,absx,absx,xxx,
	imm,indx,xxx,xxx,zp,zp,zp,xxx,imp,imm,acc,xxx,abs,abs,abs,xxx,
	rel,indy,xxx,xxx,zpx,zpx,zpx,xxx,imp,absy,acc,xxx,xxx,absx,absx,xxx
};


static int cycles;
static unsigned char bval;
static unsigned short wval;
static unsigned char a,x,y,s,p;
static unsigned short pc;

static unsigned char getaddr(int mode) {
	unsigned short ad, ad2;
	switch(mode) {
	case imp:
		cycles+=2;
		return 0;
	case imm:
		cycles+=2;
		return getmem(pc++);
	case abs:
		cycles+=4;
		ad=getmem(pc++);
		ad|=getmem(pc++)<<8;
		return getmem(ad);
	case absx:
		cycles+=4;
		ad=getmem(pc++);
		ad|=256*getmem(pc++);
		ad2=ad+x;
		if ((ad2&0xff00)!=(ad&0xff00)) cycles++;
		return getmem(ad2);
	case absy:
		cycles+=4;
		ad=getmem(pc++);
		ad|=256*getmem(pc++);
		ad2=ad+y;
		if ((ad2&0xff00)!=(ad&0xff00)) cycles++;
		return getmem(ad2);
	case zp:
		cycles+=3;
		ad=getmem(pc++);
		return getmem(ad);
	case zpx:
		cycles+=4;
		ad=getmem(pc++);
		ad+=x;
		return getmem(ad&0xff);
	case zpy:
		cycles+=4;
		ad=getmem(pc++);
		ad+=y;
		return getmem(ad&0xff);
	case indx:
		cycles+=6;
		ad=getmem(pc++);
		ad+=x;
		ad2=getmem(ad&0xff);
		ad++;
		ad2|=getmem(ad&0xff)<<8;
		return getmem(ad2);
	case indy:
		cycles+=5;
		ad=getmem(pc++);
		ad2=getmem(ad);
		ad2|=getmem((ad+1)&0xff)<<8;
		ad=ad2+y;
		if ((ad2&0xff00)!=(ad&0xff00)) cycles++;
		return getmem(ad);
	case acc:
		cycles+=2;
		return a;
	}
	return 0;
}


static void setaddr(int mode, unsigned char val) {
	unsigned short ad,ad2;
	switch(mode) {
	case abs:
		cycles+=2;
		ad=getmem(pc-2);
		ad|=256*getmem(pc-1);
		setmem(ad,val);
		return;
	case absx:
		cycles+=3;
		ad=getmem(pc-2);
		ad|=256*getmem(pc-1);
		ad2=ad+x;
		if ((ad2&0xff00)!=(ad&0xff00))
		cycles--;
		setmem(ad2,val);
		return;
	case zp:
		cycles+=2;
		ad=getmem(pc-1);
		setmem(ad,val);
		return;
	case zpx:
		cycles+=2;
		ad=getmem(pc-1);
		ad+=x;
		setmem(ad&0xff,val);
		return;
	case acc:
		a=val;
		return;
	}
}


static void putaddr(int mode, unsigned char val) {
	unsigned short ad,ad2;
	switch(mode) {
	case abs:
		cycles+=4;
		ad=getmem(pc++);
		ad|=getmem(pc++)<<8;
		setmem(ad,val);
		return;
	case absx:
		cycles+=4;
		ad=getmem(pc++);
		ad|=getmem(pc++)<<8;
		ad2=ad+x;
		setmem(ad2,val);
		return;
	case absy:
		cycles+=4;
		ad=getmem(pc++);
		ad|=getmem(pc++)<<8;
		ad2=ad+y;
		if ((ad2&0xff00)!=(ad&0xff00)) cycles++;
		setmem(ad2,val);
		return;
	case zp:
		cycles+=3;
		ad=getmem(pc++);
		setmem(ad,val);
		return;
	case zpx:
		cycles+=4;
		ad=getmem(pc++);
		ad+=x;
		setmem(ad&0xff,val);
		return;
	case zpy:
		cycles+=4;
		ad=getmem(pc++);
		ad+=y;
		setmem(ad&0xff,val);
		return;
	case indx:
		cycles+=6;
		ad=getmem(pc++);
		ad+=x;
		ad2=getmem(ad&0xff);
		ad++;
		ad2|=getmem(ad&0xff)<<8;
		setmem(ad2,val);
		return;
	case indy:
		cycles+=5;
		ad=getmem(pc++);
		ad2=getmem(ad);
		ad2|=getmem((ad+1)&0xff)<<8;
		ad=ad2+y;
		setmem(ad,val);
		return;
	case acc:
		cycles+=2;
		a=val;
		return;
	}
}


static inline void setflags(int flag, int cond) {
	if (cond) p|=flag;
	else p&=~flag;
}


static inline void push(unsigned char val) {
	setmem(0x100+s,val);
	if (s) s--;
}

static inline unsigned char pop() {
	if (s<0xff) s++;
	return getmem(0x100+s);
}

static void branch(int flag) {
	signed char dist;
	dist=(signed char)getaddr(imm);
	wval=pc+dist;
	if (flag) {
		cycles+=((pc&0x100)!=(wval&0x100))?2:1;
		pc=wval;
	}
}

// ----------------------------------------------------- ffentliche Routinen

void cpuReset() {
	a = x = y = 0;
	p = 0;
	s = 255;
	pc = getaddr(0xfffc);
}

void cpuResetTo(unsigned short npc) {
	a = 0;
	x = 0;
	y = 0;
	p = 0;
	s = 255;
	pc = npc;
}

static inline void cpuParse(void) {
	unsigned char opc = getmem(pc++);
	int cmd = opcodes[opc];
	int addr = modes[opc];
	int c;
	switch (cmd) {
	case adc:
		wval=(unsigned short)a+getaddr(addr)+((p&FLAG_C)?1:0);
		setflags(FLAG_C, wval&0x100);
		a=(unsigned char)wval;
		setflags(FLAG_Z, !a);
		setflags(FLAG_N, a&0x80);
		setflags(FLAG_V, (!!(p&FLAG_C)) ^ (!!(p&FLAG_N)));
		break;
	case and_:
		bval=getaddr(addr);
		a&=bval;
		setflags(FLAG_Z, !a);
		setflags(FLAG_N, a&0x80);
		break;
	case asl:
		wval=getaddr(addr);
		wval<<=1;
		setaddr(addr,(unsigned char)wval);
		setflags(FLAG_Z,!wval);
		setflags(FLAG_N,wval&0x80);
		setflags(FLAG_C,wval&0x100);
		break;
	case bcc:
		branch(!(p&FLAG_C));
		break;
	case bcs:
		branch(p&FLAG_C);
		break;
	case bne:
		branch(!(p&FLAG_Z));
		break;
	case beq:
		branch(p&FLAG_Z);
		break;
	case bpl:
		branch(!(p&FLAG_N));
		break;
	case bmi:
		branch(p&FLAG_N);
		break;
	case bvc:
		branch(!(p&FLAG_V));
		break;
	case bvs:
		branch(p&FLAG_V);
		break;
	case bit:
		bval=getaddr(addr);
		setflags(FLAG_Z,!(a&bval));
		setflags(FLAG_N,bval&0x80);
		setflags(FLAG_V,bval&0x40);
		break;
	case brk:
		pc=0;	/* Just quit the emulation */
		break;
	case clc:
		setflags(FLAG_C,0);
		break;
	case cld:
		setflags(FLAG_D,0);
		break;
	case cli:
		setflags(FLAG_I,0);
		break;
	case clv:
		setflags(FLAG_V,0);
		break;
	case cmp:
		bval=getaddr(addr);
		wval=(unsigned short)a-bval;
		setflags(FLAG_Z,!wval);
		setflags(FLAG_N,wval&0x80);
		setflags(FLAG_C,a>=bval);
		break;
	case cpx:
		bval=getaddr(addr);
		wval=(unsigned short)x-bval;
		setflags(FLAG_Z,!wval);
		setflags(FLAG_N,wval&0x80);
		setflags(FLAG_C,x>=bval);
		break;
	case cpy:
		bval=getaddr(addr);
		wval=(unsigned short)y-bval;
		setflags(FLAG_Z,!wval);
		setflags(FLAG_N,wval&0x80);
		setflags(FLAG_C,y>=bval);
		break;
	case dec:
		bval=getaddr(addr);
		bval--;
		setaddr(addr,bval);
		setflags(FLAG_Z,!bval);
		setflags(FLAG_N,bval&0x80);
		break;
	case dex:
		x--;
		setflags(FLAG_Z,!x);
		setflags(FLAG_N,x&0x80);
		break;
	case dey:
		y--;
		setflags(FLAG_Z,!y);
		setflags(FLAG_N,y&0x80);
		break;
	case eor:
		bval=getaddr(addr);
		a^=bval;
		setflags(FLAG_Z,!a);
		setflags(FLAG_N,a&0x80);
		break;
	case inc:
		bval=getaddr(addr);
		bval++;
		setaddr(addr,bval);
		setflags(FLAG_Z,!bval);
		setflags(FLAG_N,bval&0x80);
		break;
	case inx:
		x++;
		setflags(FLAG_Z,!x);
		setflags(FLAG_N,x&0x80);
		break;
	case iny:
		y++;
		setflags(FLAG_Z,!y);
		setflags(FLAG_N,y&0x80);
		break;
	case jmp:
		wval=getmem(pc++);
		wval|=256*getmem(pc++);
		switch (addr) {
		case abs:
			pc=wval;
			break;
		case ind:
			pc=getmem(wval);
			pc|=256*getmem(wval+1);
			break;
		}
		break;
	case jsr:
		push((pc+1)>>8);
		push((pc+1));
		wval=getmem(pc++);
		wval|=256*getmem(pc++);
		pc=wval;
		break;
	case lda:
		a=getaddr(addr);
		setflags(FLAG_Z,!a);
		setflags(FLAG_N,a&0x80);
		break;
	case ldx:
		x=getaddr(addr);
		setflags(FLAG_Z,!x);
		setflags(FLAG_N,x&0x80);
		break;
	case ldy:
		y=getaddr(addr);
		setflags(FLAG_Z,!y);
		setflags(FLAG_N,y&0x80);
		break;
	case lsr:
		bval=getaddr(addr); wval=(unsigned char)bval;
		wval>>=1;
		setaddr(addr,(unsigned char)wval);
		setflags(FLAG_Z,!wval);
		setflags(FLAG_N,wval&0x80);
		setflags(FLAG_C,bval&1);
		break;
	case nop:
		break;
	case ora:
		bval=getaddr(addr);
		a|=bval;
		setflags(FLAG_Z,!a);
		setflags(FLAG_N,a&0x80);
		break;
	case pha:
		push(a);
		break;
	case php:
		push(p);
		break;
	case pla:
		a=pop();
		setflags(FLAG_Z,!a);
		setflags(FLAG_N,a&0x80);
		break;
	case plp:
		p=pop();
		break;
	case rol:
		bval=getaddr(addr);
		c=!!(p&FLAG_C);
		setflags(FLAG_C,bval&0x80);
		bval<<=1;
		bval|=c;
		setaddr(addr,bval);
		setflags(FLAG_N,bval&0x80);
		setflags(FLAG_Z,!bval);
		break;
	case ror:
		bval=getaddr(addr);
		c=!!(p&FLAG_C);
		setflags(FLAG_C,bval&1);
		bval>>=1;
		bval|=128*c;
		setaddr(addr,bval);
		setflags(FLAG_N,bval&0x80);
		setflags(FLAG_Z,!bval);
		break;
	case rti:
		/* Treat RTI like RTS */
	case rts:
		wval=pop();
		wval|=pop()<<8;
		pc=wval+1;
		break;
	case sbc:
		bval=getaddr(addr)^0xff;
		wval=(unsigned short)a+bval+((p&FLAG_C)?1:0);
		setflags(FLAG_C, wval&0x100);
		a=(unsigned char)wval;
		setflags(FLAG_Z, !a);
		setflags(FLAG_N, a>127);
		setflags(FLAG_V, (!!(p&FLAG_C)) ^ (!!(p&FLAG_N)));
		break;
	case sec:
		setflags(FLAG_C,1);
		break;
	case sed:
		setflags(FLAG_D,1);
		break;
	case sei:
		setflags(FLAG_I,1);
		break;
	case sta:
		putaddr(addr,a);
		break;
	case stx:
		putaddr(addr,x);
		break;
	case sty:
		putaddr(addr,y);
		break;
	case tax:
		x=a;
		setflags(FLAG_Z, !x);
		setflags(FLAG_N, x&0x80);
		break;
	case tay:
		y=a;
		setflags(FLAG_Z, !y);
		setflags(FLAG_N, y&0x80);
		break;
	case tsx:
		x=s;
		setflags(FLAG_Z, !x);
		setflags(FLAG_N, x&0x80);
		break;
	case txa:
		a=x;
		setflags(FLAG_Z, !a);
		setflags(FLAG_N, a&0x80);
		break;
	case txs:
		s=x;
		break;
	case tya:
		a=y;
		setflags(FLAG_Z, !a);
		setflags(FLAG_N, a&0x80);
		break;
	}
}

void cpuJSR(unsigned short npc, unsigned char na) {
	a = na;
	x = 0;
	y = 0;
	p = 0;
	s = 255;
	pc = npc;
	push(0);
	push(0);
	while(pc) cpuParse();
}


void c64Init(void) {
	memset(memory, 0, sizeof(memory));
	cpuReset();
}

unsigned short LoadSIDFromMemory(void *pSidData, unsigned short *load_addr,
	unsigned short *init_addr, unsigned short *play_addr,
	unsigned char *subsongs, unsigned char *startsong, unsigned char *speed,
	unsigned short size) {

	unsigned char *pData;
	unsigned char data_file_offset;

	pData = (unsigned char*)pSidData;
	data_file_offset = pData[7];

	*load_addr = pData[8]<<8;
	*load_addr|= pData[9];

	*init_addr = pData[10]<<8;
	*init_addr|= pData[11];

	*play_addr = pData[12]<<8;
	*play_addr|= pData[13];

	*subsongs = pData[0xf]-1;
	*startsong = pData[0x11]-1;

	*load_addr = pData[data_file_offset];
	*load_addr|= pData[data_file_offset+1]<<8;

	*speed = pData[0x15];

	memset(memory, 0, sizeof(memory));
	memcpy(&memory[*load_addr], &pData[data_file_offset+2], size-(data_file_offset+2));

	if(!*play_addr) {
		cpuJSR(*init_addr, 0);
		*play_addr = (memory[0x0315]<<8)+memory[0x0314];
	}

	return *load_addr;
}

unsigned short c64SidLoad(const char* filename, unsigned short* init_addr,
	unsigned short* play_addr, unsigned char* sub_song_start,
	unsigned char* max_sub_songs, unsigned char* speed, char* name,
	char* author, char* copyright) {

	int i;
	FILE* f = fopen(filename, "rb");
	if(!f) return 0;

	// Check header
	char PSID[4];
	if(!fread(PSID, 4, 1, f) && strncmp(PSID, "PSID", 4)) return 0;

	// Name holen
	fseek(f, 0x16, 0);
	for(i = 0; i < 32; i++) name[i] = fgetc(f);

	// Author holen
	fseek(f, 0x36, 0);
	for(i = 0; i < 32; i++) author[i] = fgetc(f);

	// Copyright holen
	fseek(f, 0x56, 0);
	for(i = 0; i < 32; i++) copyright[i] = fgetc(f);
	fclose(f);


	unsigned char sidmem[65536];
	unsigned char *p = sidmem;
	unsigned short load_addr;

	if( (f=fopen(filename, "rb")) == NULL) return 0;
	while(!feof(f)) *p++ = fgetc(f);
	fclose(f);

	LoadSIDFromMemory(sidmem, &load_addr,
		init_addr, play_addr, max_sub_songs, sub_song_start, speed, p-sidmem);

	return load_addr;
}

