The header stplugin.h and the file stplugin.c must be in the current directory. 
Then:

gcc -bundle -DSYSTEM=APPLEMAC  -I. -I/sw/include -L/sw/lib stplugin.c allSubsetsMeta.c -o allSubsetsMeta.plugin -lm -lgsl -lgslcblas

and the C plugin is ready.