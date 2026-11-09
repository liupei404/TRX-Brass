#include <stdint.h>
#include <complex.h>

#define FFT_BITS    11
#define FFT_SAMPLES (1 << FFT_BITS)

typedef struct {
    uint32_t        count;
    float complex   factor[1 << (FFT_BITS - 1)];
    float complex   samples[FFT_SAMPLES];
    float           window[FFT_SAMPLES];
    float           out[FFT_SAMPLES];
} mb_data_t;
