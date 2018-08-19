CFLAGS = -g

gpmfdemo : GPMF_demo.o GPMF_parser.o GPMF_mp4reader.o GPMF_print.o
		gcc $(CFLAGS) -o gpmfdemo GPMF_demo.o GPMF_parser.o GPMF_mp4reader.o GPMF_print.o

GPMF_demo.o : GPMF_demo.c ../GPMF_parser.h
		gcc $(CFLAGS) -c GPMF_demo.c
GPMF_mp4reader.o : GPMF_mp4reader.c ../GPMF_parser.h
		gcc $(CFLAGS) -c GPMF_mp4reader.c
GPMF_print.o : GPMF_print.c ../GPMF_parser.h
		gcc $(CFLAGS) -c GPMF_print.c
GPMF_parser.o : ../GPMF_parser.c ../GPMF_parser.h
		gcc $(CFLAGS) -c ../GPMF_parser.c
clean :
		rm gpmfdemo GPMF_demo.o GPMF_parser.o GPMF_mp4reader.o GPMF_print.o