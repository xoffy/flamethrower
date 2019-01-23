
#ifndef __NOISE_H_
#define __NOISE_H_

void noise_init();
double noise(double x);
double noise2(double x, double amplitude, double scale);

extern double noise_amplitude;
extern double noise_scale;

#endif

