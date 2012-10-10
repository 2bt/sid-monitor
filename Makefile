LIBS = -lSDL -lSDL_image

all: resid-0.16/.libs/libresid.a
	g++ -Wall -O2 main.cpp 6502.c resid-0.16/.libs/libresid.a $(LIBS) -o monitor


resid-0.16/.libs/libresid.a:
	cd resid-0.16 && ./configure
	make -C resid-0.16

clean:
	rm -f monitor
	make -C resid-0.16 distclean
