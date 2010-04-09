objects = allSubsetsMeta.o stplugin.o

allSubsetsMeta.plugin : $(objects) 
	gcc  -bundle -L/sw/lib -lm -lgsl -lgslcblas \
		-o allSubsetsMeta.plugin $(objects) 

stplugin.o : stplugin.c stplugin.h
	gcc -c stplugin.c -I. -I/sw/include \
		-DSYSTEM=APPLEMAC \
		 -Wall -W \
		-Wmissing-prototypes -Wstrict-prototypes \
		-Wconversion -Wshadow \
		-Wpointer-arith -Wcast-qual -Wcast-align \
		-Wwrite-strings -Wnested-externs \
		-fshort-enums -fno-common -g -O3 

allSubsetsMeta.o : allSubsetsMeta.c stplugin.h
	gcc -c allSubsetsMeta.c -I. -I/sw/include \
		-DSYSTEM=APPLEMAC \
		 -Wall -W \
		-Wmissing-prototypes -Wstrict-prototypes \
		-Wconversion -Wshadow \
		-Wpointer-arith -Wcast-qual -Wcast-align \
		-Wwrite-strings -Wnested-externs \
		-fshort-enums -fno-common -g -O3 


.PHONY : clean
clean : 
	rm $(objects)
