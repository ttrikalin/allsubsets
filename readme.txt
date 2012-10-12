/* USING THE PRECOMPILED BINARIES */ 

... may work for you, or not, depending on your system's details. 

-- On my mac the plugin (allsubsetsmeta_osx.plugin) works fine for OS 10.5. through 10.7.4. 

-- On Windows, the plugin (allsubsetsmeta_win.plugin) works on 32 bit XP (home something or other) and 32 bit Windows 7. I do not have a 64 bit machine, so dunno whether it works there. 

-- If the binaries do not work for you, then do try compile from source before shooting me an email (thomas_trikalinos [at] REMOVETRASHCAPS brown [dot] edu).  
Compiling should be easy -- the only dependency is the (open source and free) GNU Scientific Library (GSL) -- just because I got lazy and used a couple of functions from there.  

take care, 

tom in MA



/* COMPILING */

/* Mac OSX */ 

The header stplugin.h and the file stplugin.c must be in the current directory. 
Then:

gcc -bundle -DSYSTEM=APPLEMAC  -I. -I/sw/include -L/sw/lib stplugin.c allSubsetsMeta.c -o allsubsetsmeta_osx.plugin -lm -lgsl -lgslcblas

and the C plugin is ready.

/* Windowz -- through Cygwin gcc */

gcc -shared -mno-cygwin stplugin.c allSubsetsMeta.c -o allsubsetsmeta_win.plugin -lm -lgsl -lgslcblas

 



