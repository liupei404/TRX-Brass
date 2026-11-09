/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 *
 *  Based on WDSP lib
 *  Copyright (C) 2011 - 2017 Warren Pratt, NR0V
 */

#pragma once

#include <fftw3.h>

typedef enum {
    EMNR_GAIN_GAUSIAN_LINEAR = 0,
    EMNR_GAIN_GAUSIAN_LOG,
    EMNR_GAIN_GAMMA,
    EMNR_GAIN_TRAINED
} emnr_gain_t;

typedef enum {
    EMNR_NPE_OSMS = 0,
    EMNR_NPE_MMSE,
    EMNR_NPE_NSTAT,
} emnr_npe_t;

typedef struct {
    int       bsize;
    float    *in;
    float    *out;
    int       fsize;
    int       ovrlp;
    int       incr;
    double   *window;
    int       iasize;
    float    *inaccum;
    double   *forfftin;
    double   *forfftout;
    int       msize;
    double   *mask;
    double   *revfftin;
    double   *revfftout;
    double  **save;
    int       oasize;
    double   *outaccum;
    double    rate;
    int       wintype;
    double    ogain;
    double    gain;
    int       nsamps;
    int       iainidx;
    int       iaoutidx;
    int       init_oainidx;
    int       oainidx;
    int       oaoutidx;
    int       saveidx;
    fftw_plan Rfor;
    fftw_plan Rrev;

    struct {
        emnr_gain_t gain_method;
        emnr_npe_t  npe_method;
        int         ae_run;
        double      msize;
        double     *mask;
        double     *y;
        double     *lambda_y;
        double     *lambda_d;
        double     *prev_mask;
        double     *prev_gamma;
        double      gf1p5;
        double      alpha;
        double      eps_floor;
        double      gamma_max;
        double      xi_min;
        double      q;
        double      gmax;
        double     *GG;
        double     *GGS;
        double      zeta_thresh;
    } g;

    struct {
        int      incr;
        double   rate;
        int      msize;
        double  *lambda_y;
        double  *lambda_d;
        double  *p;
        double  *alphaOptHat;
        double   alphaC;
        double   alphaCsmooth;
        double   alphaCmin;
        double  *alphaHat;
        double   alphaMax;
        double  *sigma2N;
        double   alphaMin_max_value;
        double   snrq;
        double   betamax;
        double  *pbar;
        double  *p2bar;
        double   invQeqMax;
        double   av;
        double  *Qeq;
        int      U;
        double   Dtime;
        int      V;
        int      D;
        double   MofD;
        double   MofV;
        double  *bmin;
        double  *bmin_sub;
        int     *k_mod;
        double  *actmin;
        double  *actmin_sub;
        int      subwc;
        int     *lmin_flag;
        double  *pmin_u;
        double   invQbar_points[4];
        double   nsmax[4];
        double **actminbuff;
        int      amb_idx;
    } np;

    struct {
        int     incr;
        double  rate;
        int     msize;
        double *lambda_y;
        double *lambda_d;

        double  alpha_pow;
        double  alpha_Pbar;
        double  epsH1;
        double  epsH1r;

        double *sigma2N;
        double *PH1y;
        double *Pbar;
        double *EN2y;
    } nps;

    struct {
        double  rate;
        int     msize;
        int     incr;
        double *Ysq;
        double *P;
        double *Pmin;
        double *p;
        double *D;
        double *lambda_d;

        double  eta;
        double  gamma;
        double  beta;
        double  delta_LF;
        double  delta_MF;
        double  delta_0;
        double  delta_1;
        double  delta_2;
        double  alpha_d;
        double  alpha_p;
    } npl;

    struct {
        int     msize;
        double *lambda_y;
        double  zetaThresh;
        double  psi;
        double *nmask;
        double  t2;
    } ae;
} emnr_t;

emnr_t *emnr_create(int size, float *in, float *out, int fsize, int ovrlp, int rate, int wintype, double gain,
                    emnr_gain_t gain_method, emnr_npe_t npe_method, int ae_run);

void    emnr_apply(emnr_t *a);
void    emnr_destroy(emnr_t *a);
void    emnr_flush(emnr_t *a);

void    emnr_set_gain_method(emnr_t *a, emnr_gain_t x);
void    emnr_set_npe_method(emnr_t *a, emnr_npe_t x);
void    emnr_set_trained_thresh(emnr_t *a, float x);
void    emnr_set_trained_t2(emnr_t *a, float x);
