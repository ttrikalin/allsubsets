#include <stdlib.h>
#include "stplugin.h"
#include <string.h>

#include <gsl/gsl_combination.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_math.h>

#define c(i) (gsl_combination_get(comb, (i)))
#define MARES(i) (gsl_vector_get(metaResultsVector, (i)))

/* 10/15/2012 Silly trick to export symbol in MS compiler 
   (Windowz VS 2008 C/C++)*/

#if _MSC_VER
#define snprintf _snprintf 
#endif 


/* The plugin should be called with the number of studies in the meta as 1st arg*/

int AllSubsetsMetaAnalysis(gsl_vector * esVector, gsl_vector * varVector, 
	gsl_vector * metaResultsVector);

int MetaAnalysis(gsl_vector * esVector, gsl_vector * varVector, 
	gsl_vector * metaResultsVector, gsl_combination * comb);

int DL(gsl_vector * esVector, gsl_vector * varVector, 
	gsl_vector * metaResultsVector , gsl_combination * comb, 
	const ST_double sumOfFixedWeights,
	const ST_double sumOfFixedWeights2 );

int WriteOut(gsl_vector * metaResultsVector, ST_long j, gsl_combination * comb);

/*
char *itoa(int n, char *s, int b); 
char *strrev(char *str);
*/

STDLL stata_call(int argc, char *argv[]) {
	ST_uint4	j;
	ST_uint4 	n;
	ST_long 	nSubsets;
	ST_uint4 	nOutputVars=9; /* ES_F, varES_F, es_R, varES_R, df, Q, I2, tau2 subsetString */
	ST_uint4 	nInputVars=2;  /* ES, var*/
	ST_double	z, es, var;
	ST_retcode	rc;
	ST_double 	version = 0.1;	
	
	char	buf[80];
	
	gsl_vector * esVector;
	gsl_vector * varVector;
	gsl_vector * metaResultsVector;
	
	if (argc != 1) {
		snprintf(buf, 80, "Plugin (ver: %g) usage: c_meta nStudies\n", version);
		SF_error(buf);
		SF_error("Please debug the calling .ado file!!!\n");
		return (-1);
	}
	n = (ST_uint4) atoi(argv[0]);
	esVector = gsl_vector_alloc(n);
	varVector = gsl_vector_alloc(n);

	/* This will hold the meta-analysis results */
	metaResultsVector = gsl_vector_alloc(nOutputVars-1);  /* last var is string */
	
	nSubsets = gsl_pow_int(2,n)-1;
	if (SF_in2()<nSubsets) {
		snprintf(buf, 80, "Plugin needs %ld observations in dataset\n", nSubsets);
		SF_error(buf);
		SF_error("Please debug the calling .ado file!!!\n");
		return (-1);
	}
	
	if (SF_nvars() != nInputVars+nOutputVars)  {
		snprintf(buf, 80, "Plugin expects %d input and %d output variables!\n",
			nInputVars, nOutputVars);
		SF_error(buf);
		SF_error("Please debug the calling .ado file!!!\n");
		return(102);
	}
    
	for(j=SF_in1(); j<= n ; j++) {
		if (SF_ifobs(j)) {
			if(rc=SF_vdata(1,j,&z)) return(rc);
			es = z;
			if(rc=SF_vdata(2,j,&z)) return(rc);
			var = z;
			if(SF_is_missing(es) | SF_is_missing(var)) {
				snprintf(buf, 80,"You have passed missing values (obs # %d) in the C plugin!\n", j);
				SF_error(buf);
				SF_error("Please debug the calling .ado file!!!\n");
				return(-1);
			} 
			else {
				/* do stuff here */
				gsl_vector_set(esVector,j-1, es);
				gsl_vector_set(varVector,j-1, var);
			}
		}
	}

	if (rc=AllSubsetsMetaAnalysis(esVector, varVector, metaResultsVector)) return(rc); 

	gsl_vector_free(esVector);
	gsl_vector_free(varVector);
	gsl_vector_free(metaResultsVector);
	
	return(0);
}

int AllSubsetsMetaAnalysis(gsl_vector * esVector, gsl_vector * varVector,
	gsl_vector * metaResultsVector) {
	ST_retcode	rc;
	ST_uint4 	i, nStudies;
	ST_long		j, nSubsets;
	char 		buf[80];
	
	gsl_combination * comb;

	nStudies = esVector->size;
	nSubsets = gsl_pow_int(2, nStudies)-1 ;


	j=1;
	for(i=1; i <= nStudies; i++) { 
		
		comb = 	gsl_combination_calloc(nStudies, i);
		
		do { 
			if(j == nSubsets+1) {
				snprintf(buf, 80,"combLength %u Obs %u\n",i,  j);
				SF_error(buf);
				SF_error("Exceeded the maximum number of subsets!!!\n");
				return(-2);
			}
			if (rc = MetaAnalysis(esVector, varVector, metaResultsVector, comb) ) return(rc);
			if (rc = WriteOut(metaResultsVector, j, comb) ) return(rc);
			j += 1;
		} while (gsl_combination_next(comb) == GSL_SUCCESS);
	}

	gsl_combination_free(comb);
	return(0);
}

int MetaAnalysis(gsl_vector * esVector, gsl_vector * varVector, 
	gsl_vector * metaResultsVector, gsl_combination * comb) {
	
	ST_retcode	rc;
	ST_uint4 	i, nStudies, subsetLength;
	
	ST_double sumOfFixedWeights = 0.0;
	ST_double sumOfFixedWeights2= 0.0;
	ST_double sumOfFixedWeightedEffects = 0.0;
	ST_double sumOfFixedWeightedSquares = 0.0;
	
	
	nStudies = (ST_uint4) esVector->size;

	subsetLength = gsl_combination_k(comb);
	
        
/* note the definition of c(i) in the beginning */	
	if (subsetLength > 1.0) {
		for(i=0; i< subsetLength ; i++) {
			sumOfFixedWeights += 1.0 / (gsl_vector_get(varVector, c(i) ));
			sumOfFixedWeights2 += 1.0 / gsl_pow_2( (gsl_vector_get(varVector, c(i) )) );
			sumOfFixedWeightedEffects += gsl_vector_get(esVector, c(i))
				/ (gsl_vector_get(varVector, c(i)));
		}
	
		/* ES_FEM */
		gsl_vector_set(metaResultsVector, 0,
			sumOfFixedWeightedEffects / sumOfFixedWeights);
		/* var_FEM*/
		gsl_vector_set(metaResultsVector, 1, 1.0 / sumOfFixedWeights);
		/* df */
		gsl_vector_set(metaResultsVector, 4, subsetLength-1.0);

		/* Q */
		for(i=0; i< subsetLength ; i++) {
			sumOfFixedWeightedSquares +=   		/* see the definition of MARES */
				gsl_pow_2(gsl_vector_get(esVector, c(i)) - MARES(0))
				/ gsl_vector_get(varVector,c(i));
		}
		gsl_vector_set(metaResultsVector, 5, sumOfFixedWeightedSquares);

		/* I2 */
		gsl_vector_set(metaResultsVector, 6, GSL_MAX(0.0, 1.0 - MARES(4) / MARES(5)) );
	
		/****REM****/
		/* sets ES_REM var_REM and tau2 */
		if (rc = DL(esVector, varVector, metaResultsVector, comb,
			sumOfFixedWeights, sumOfFixedWeights2) ) return(rc);
	}
	else {
		gsl_vector_set(metaResultsVector, 2, MARES(0));
		gsl_vector_set(metaResultsVector, 3, MARES(1));
		gsl_vector_set(metaResultsVector, 5, 0.0);   /* Q will set to missing later */
		gsl_vector_set(metaResultsVector, 6, 0.0 );   /* I2 will set to missing later */
		gsl_vector_set(metaResultsVector, 7, 0.0 );     /* no tau2 */
		
	}
	return 0;
	

}

int DL(gsl_vector * esVector, gsl_vector * varVector, 
	gsl_vector * metaResultsVector , gsl_combination * comb, 
	const ST_double sumOfFixedWeights,
	const ST_double sumOfFixedWeights2 ) {
	
	ST_double tau2_DL;
	ST_double sumOfRandomDLWeights = 0.0;
	ST_double sumOfRandomDLWeightedEffects = 0.0;
	ST_double sumOfRandomDLWeightedSquares = 0.0;

	ST_uint4 nStudies = (ST_uint4) esVector->size ;
	ST_uint4 i , subsetLength;
	
	subsetLength = (ST_uint4) gsl_combination_k(comb);
	
	if (subsetLength < 2) {
		SF_error("Never here with one study! Please debug C code");
		return(-4);
	}
	else {
                tau2_DL = GSL_MAX((MARES(5) - MARES(4)) /
                        (sumOfFixedWeights - (sumOfFixedWeights2 / sumOfFixedWeights) ) , 0.0 );
		gsl_vector_set(metaResultsVector, 7, tau2_DL);

                for(i=0; i<subsetLength ; i++) {
                        sumOfRandomDLWeights += 1.0 / (gsl_vector_get(varVector, c(i)) + tau2_DL);
                        sumOfRandomDLWeightedEffects += gsl_vector_get(esVector, c(i))
                                / (gsl_vector_get(varVector, c(i)) + tau2_DL);
                }

                /* Summary ES REM-DL */
		gsl_vector_set(metaResultsVector, 2, 
			sumOfRandomDLWeightedEffects / sumOfRandomDLWeights);

                /* Var of summary ES REM-DL */
		gsl_vector_set(metaResultsVector, 3, 
			1.0 / sumOfRandomDLWeights);
	
	}
	return 0;
}



int WriteOut(gsl_vector * metaResultsVector, ST_long j, gsl_combination * comb) {
	
	ST_retcode	rc;
	ST_uint4	i, subsetLength = gsl_combination_k(comb);
	char 		digits[52]="abcdefghijklmnopqrstuvwxywABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char 		buf[80] ,temp[80];
	
	if(rc = SF_vstore(3, j, MARES(0))) return(rc);  /* ES_FEM */
	if(rc = SF_vstore(4, j, MARES(1))) return(rc);  /* var_FEM */
	if(rc = SF_vstore(5, j, MARES(2))) return(rc);  /* ES_REM */
	if(rc = SF_vstore(6, j, MARES(3))) return(rc);  /* var_REM */
	if(rc = SF_vstore(7, j, MARES(4))) return(rc);  /* df */
	if (MARES(4) > 0.0) {
		if(rc = SF_vstore(8, j, MARES(5))) return(rc);  /* Q */
		if(rc = SF_vstore(9, j, MARES(6))) return(rc);  /* I2 */
	}
	else {
		if(rc = SF_vstore(8, j, SV_missval )) return(rc);  /* Q */
		if(rc = SF_vstore(9, j, SV_missval )) return(rc);  /* I2 */
	}
	if(rc = SF_vstore(10, j, MARES(7))) return(rc);  /* tau2 */
	
	if (subsetLength>80) {
		SF_error("Exceeded 80 characters for subset description");
		SF_error("I will not print the string variable coding the subsets");
	}
	else {
		for(i=0; i<subsetLength; i++) {
			buf[i]=digits[c(i)];
			if (i==subsetLength-1) {
				buf[i+1]='\0';
			}
		}
	}
	
	if(rc = SF_sstore(11, j, buf)) return(rc);  


	return 0;
}

/*
char *strrev(char *str) {
	char *p1, *p2;

	if (!str || !*str)
		return str;

	for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}

	return str;
}

char *itoa(int n, char *s, int b) {
	static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	int i=0, sign;
    
	if ((sign = n) < 0)
		n = -n;

	do {
		s[i++] = digits[n % b];
	} while ((n /= b) > 0);

	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';

	return strrev(s);
}
*/
