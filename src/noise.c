
#include <math.h>
#include <stdlib.h>
#include "util.h"

#define MAX_VERTICES    256
#define MAX_VERTICES_MASK (MAX_VERTICES - 1)
double rnd[MAX_VERTICES];
double noise_amplitude = 1.0;
double noise_scale = 1.0;

void noise_init() {
    unsigned int i;
    
    for (i = 0; i < MAX_VERTICES; i++) {
        rnd[i] = FRAND();
    }
}

double noise(double x) {
    double xs, t, ts, y;
    int xf, xmin, xmax;
    
    xs = x * noise_scale;
    xf = floor(xs);
    t = xs - xf;
    ts = t * t * (3 - 2 * t);
    xmin = xf & MAX_VERTICES_MASK;
    xmax = (xmin + 1) & MAX_VERTICES_MASK;
    y = LERP(rnd[xmin], rnd[xmax], ts);
    
    return y * noise_amplitude;
}

