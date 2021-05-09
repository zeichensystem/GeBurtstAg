#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>


// fracional bits of fixed point num
#define RECIPROCAL_FRACT_SHIFT 10
#define RECIPROCAL_FRACT_SIZE (1 << RECIPROCAL_FRACT_SHIFT)
// the integer bits
#define RECIPROCAL_INT_SHIFT (32 - RECIPROCAL_FRACT_SHIFT)
#define RECIPROCAL_INT_SIZE (1 << RECIPROCAL_INT_SHIFT)

#define RECIPROCAL_MAX_INT (128)
// length of resulting reciprocal LUT 
#define RECIPROCAL_LUT_SIZE (RECIPROCAL_FRACT_SIZE * RECIPROCAL_MAX_INT) 

int32_t divLutArray[RECIPROCAL_LUT_SIZE];

int32_t double2fx(double n) {
	return n * RECIPROCAL_FRACT_SIZE;
}
double fx2double(uint32_t fx) {
	return fx / (double) RECIPROCAL_FRACT_SIZE;
}


void setupDivLut() {
    assert(RECIPROCAL_FRACT_SHIFT + RECIPROCAL_INT_SHIFT == 32);
    assert((RECIPROCAL_LUT_SIZE & (RECIPROCAL_LUT_SIZE - 1)) == 0 ); // must be power of two 

    FILE *outfile = fopen("math_divlut.c", "w"); // remove existing file first
    fclose(outfile);
    outfile = fopen("math_divlut.c", "a");

    FILE *outfileHeader = fopen("math_divlut.h", "w"); // remove existing file first
    fclose(outfileHeader);
    outfileHeader = fopen("math_divlut.h", "a");

    // calculate the LUT
    for (uint32_t i = 1; i <= RECIPROCAL_LUT_SIZE; ++i) {
       divLutArray[i - 1] = double2fx(1. / fx2double(i));
    }

    // format it into a header file (metaprogramming lol)
    const char *headers = {
        "#ifndef MAT_DIVLUT_H\n"
        "#define MAT_DIVLUT_H\n"
        "#include <tonc_math.h>\n\n"
        "#define RECIPROCAL_FRACT_SHIFT %d\n"
        "#define RECIPROCAL_FRACT_SIZE %d\n"
        "#define RECIPROCAL_MAX_INT %d\n"
        "#define RECIPROCAL_LUT_SIZE %d\n\n"
        ""
        "extern const s32 reciprocalLUT[RECIPROCAL_LUT_SIZE];"
        "\n\n#endif"
    };
    fprintf(outfileHeader, headers, RECIPROCAL_FRACT_SHIFT, RECIPROCAL_FRACT_SIZE, RECIPROCAL_MAX_INT, RECIPROCAL_LUT_SIZE);
    fclose(outfileHeader);

    fprintf(outfile, "#include \"../source/math_divlut.h\"\nconst s32 reciprocalLUT[RECIPROCAL_LUT_SIZE] = {"); // u16 vs fixed 
    for (int i = 0; i < RECIPROCAL_LUT_SIZE; ++i) {
	    fprintf(outfile, "%d, ", divLutArray[i]);
    }
    fprintf(outfile, "};");
    fclose(outfile);
}


int main(void) {
	setupDivLut();
	return 0;
}
