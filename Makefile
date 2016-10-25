LF = resid-0.16/.libs/libresid.a -lSDL -lSDL_image
TRG = monitor
SRC = src/main.cpp src/record.cpp src/6502.c

all: $(TRG)

$(TRG): resid-0.16/.libs/libresid.a $(SRC) Makefile
	g++ -Wall -O2 -I. $(SRC) $(LF) -o $@


resid-0.16/.libs/libresid.a:
	cd resid-0.16 && ./configure
	make -C resid-0.16

clean:
	rm -f $(TRG)
	make -C resid-0.16 distclean
