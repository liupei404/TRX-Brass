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

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "emnr.h"
#include "zeta_hat.h"

extern double GG[];
extern double GGS[];

typedef double complex[2];

/********************************************************************************************************
 *																										*
 *											Special Functions											*
 *																										*
 ********************************************************************************************************/

// MODIFIED BESSEL FUNCTIONS OF THE 0TH AND 1ST ORDERS, Polynomial Approximations
// M. Abramowitz and I. Stegun, Eds., "Handbook of Mathematical Functions."  Washington, DC:  National
//      Bureau of Standards, 1964.
// Shanjie Zhang and Jianming Jin, "Computation of Special Functions."  New York, NY, John Wiley and Sons,
//      Inc., 1996.  [Sample code given in FORTRAN]


double bessI0(double x) {
    double res, p;

    if (x == 0.0) {
        res = 1.0;
    } else {
        if (x < 0.0) {
            x = -x;
        }

        if (x <= 3.75) {
            p   = x / 3.75;
            p   = p * p;
            res = (((((0.0045813 * p + 0.0360768) * p + 0.2659732) * p + 1.2067492) * p + 3.0899424) * p + 3.5156229) *
                      p +
                  1.0;
        } else {
            p   = 3.75 / x;
            res = exp(x) / sqrt(x) *
                  ((((((((+0.00392377 * p - 0.01647633) * p + 0.02635537) * p - 0.02057706) * p + 0.00916281) * p -
                      0.00157565) *
                         p +
                     0.00225319) *
                        p +
                    0.01328592) *
                       p +
                   0.39894228);
        }
    }

    return res;
}

double bessI1(double x) {
    double res, p;

    if (x == 0.0) {
        res = 0.0;
    } else {
        if (x < 0.0) {
            x = -x;
        }

        if (x <= 3.75) {
            p   = x / 3.75;
            p   = p * p;
            res = x * ((((((0.00032411 * p + 0.00301532) * p + 0.02658733) * p + 0.15084934) * p + 0.51498869) * p +
                        0.87890594) *
                           p +
                       0.5);
        } else {
            p   = 3.75 / x;
            res = exp(x) / sqrt(x) *
                  ((((((((-0.00420059 * p + 0.01787654) * p - 0.02895312) * p + 0.02282967) * p - 0.01031555) * p +
                      0.00163801) *
                         p -
                     0.00362018) *
                        p -
                    0.03988024) *
                       p +
                   0.39894228);
        }
    }

    return res;
}

// EXPONENTIAL INTEGRAL, E1(x)
// M. Abramowitz and I. Stegun, Eds., "Handbook of Mathematical Functions."  Washington, DC:  National
//      Bureau of Standards, 1964.
// Shanjie Zhang and Jianming Jin, "Computation of Special Functions."  New York, NY, John Wiley and Sons,
//      Inc., 1996.  [Sample code given in FORTRAN]

double e1xb(double x) {
    double e1, ga, r, t, t0;
    int    k, m;

    if (x == 0.0) {
        e1 = 1.0e300;
    } else if (x <= 1.0) {
        e1 = 1.0;
        r  = 1.0;

        for (k = 1; k <= 25; k++) {
            r  = -r * k * x / ((k + 1.0) * (k + 1.0));
            e1 = e1 + r;
            if (fabs(r) <= fabs(e1) * 1.0e-15) {
                break;
            }
        }

        ga = 0.5772156649015328;
        e1 = -ga - log(x) + x * e1;
    } else {
        m  = 20 + (int) (80.0 / x);
        t0 = 0.0;

        for (k = m; k >= 1; k--) {
            t0 = (double) k / (1.0 + k / (x + t0));
        }

        t  = 1.0 / (x + t0);
        e1 = exp(-x) * t;
    }

    return e1;
}

/********************************************************************************************************
 *																										*
 *											Main Body of Code											*
 *																										*
 ********************************************************************************************************/

void calc_window(emnr_t *a) {
    int    i;
    double arg, sum, inv_coherent_gain;

    switch (a->wintype) {
        case 0:
            arg = 2.0 * M_PI / (double) a->fsize;
            sum = 0.0;

            for (i = 0; i < a->fsize; i++) {
                a->window[i] = sqrt(0.54 - 0.46 * cos((double) i * arg));
                sum += a->window[i];
            }

            inv_coherent_gain = (double) a->fsize / sum;

            for (i = 0; i < a->fsize; i++) {
                a->window[i] *= inv_coherent_gain;
            }
            break;
    }
}

void interpM(double *res, double x, int nvals, double *xvals, double *yvals) {
    if (x <= xvals[0]) {
        *res = yvals[0];
    } else if (x >= xvals[nvals - 1]) {
        *res = yvals[nvals - 1];
    } else {
        int    idx = 0;
        double xllow, xlhigh, frac;

        while (x >= xvals[idx]) {
            idx++;
        }

        xllow  = log10(xvals[idx - 1]);
        xlhigh = log10(xvals[idx]);
        frac   = (log10(x) - xllow) / (xlhigh - xllow);
        *res   = yvals[idx - 1] + frac * (yvals[idx] - yvals[idx - 1]);
    }
}

void calc_emnr(emnr_t *a) {
    int    i;
    double Dvals[18] = {1.0,  2.0,  5.0,   8.0,   10.0,  15.0,  20.0,  30.0,  40.0,
                        60.0, 80.0, 120.0, 140.0, 160.0, 180.0, 220.0, 260.0, 300.0};
    double Mvals[18] = {0.000, 0.260, 0.480, 0.580, 0.610, 0.668, 0.705, 0.762, 0.800,
                        0.841, 0.865, 0.890, 0.900, 0.910, 0.920, 0.930, 0.935, 0.940};
    double Hvals[18] = {0.000, 0.150, 0.480, 0.780, 0.980, 1.550, 2.000, 2.300, 2.520,
                        3.100, 3.380, 4.150, 4.350, 4.250, 3.900, 4.100, 4.700, 5.000};

    a->incr          = a->fsize / a->ovrlp;
    a->gain          = a->ogain / a->fsize / (double) a->ovrlp;

    if (a->fsize > a->bsize) {
        a->iasize = a->fsize;
    } else {
        a->iasize = a->bsize + a->fsize - a->incr;
    }

    a->iainidx  = 0;
    a->iaoutidx = 0;

    if (a->fsize > a->bsize) {
        if (a->bsize > a->incr) {
            a->oasize = a->bsize;
        } else {
            a->oasize = a->incr;
        }
        a->oainidx = (a->fsize - a->bsize - a->incr) % a->oasize;
    } else {
        a->oasize  = a->bsize;
        a->oainidx = a->fsize - a->incr;
    }

    a->init_oainidx = a->oainidx;
    a->oaoutidx     = 0;
    a->msize        = a->fsize / 2 + 1;
    a->window       = (double *) calloc(a->fsize, sizeof(double));
    a->inaccum      = (float *) calloc(a->iasize, sizeof(float));
    a->forfftin     = (double *) calloc(a->fsize, sizeof(double));
    a->forfftout    = (double *) calloc(a->msize, sizeof(complex));
    a->mask         = (double *) calloc(a->msize, sizeof(double));
    a->revfftin     = (double *) calloc(a->msize, sizeof(complex));
    a->revfftout    = (double *) calloc(a->fsize, sizeof(double));
    a->save         = (double **) calloc(a->ovrlp, sizeof(double *));

    for (i = 0; i < a->ovrlp; i++) {
        a->save[i] = (double *) calloc(a->fsize, sizeof(double));
    }

    a->outaccum = (double *) calloc(a->oasize, sizeof(double));
    a->nsamps   = 0;
    a->saveidx  = 0;
    a->Rfor     = fftw_plan_dft_r2c_1d(a->fsize, a->forfftin, (fftw_complex *) a->forfftout, FFTW_ESTIMATE);
    a->Rrev     = fftw_plan_dft_c2r_1d(a->fsize, (fftw_complex *) a->revfftin, a->revfftout, FFTW_ESTIMATE);
    calc_window(a);

    /* g */

    a->g.msize      = a->msize;
    a->g.mask       = a->mask;
    a->g.y          = a->forfftout;
    a->g.lambda_y   = (double *) calloc(a->msize, sizeof(double));
    a->g.lambda_d   = (double *) calloc(a->msize, sizeof(double));
    a->g.prev_gamma = (double *) calloc(a->msize, sizeof(double));
    a->g.prev_mask  = (double *) calloc(a->msize, sizeof(double));

    a->g.gf1p5      = sqrt(M_PI) / 2.0;
    {
        double tau = -128.0 / 8000.0 / log(0.985);
        a->g.alpha = exp(-a->incr / a->rate / tau);
    }
    a->g.eps_floor = 1.0e-300;
    a->g.gamma_max = 40.0;
    a->g.xi_min    = pow(10.0, -40.0 / 10.0);
    a->g.q         = 0.2;

    for (i = 0; i < a->g.msize; i++) {
        a->g.prev_mask[i]  = 1.0;
        a->g.prev_gamma[i] = 1.0;
    }

    a->g.gmax = 10000.0;

    /* * */

    a->g.GG = GG;
    a->g.GGS = GGS;
    a->g.zeta_thresh = -5.0;

    /* np */

    a->np.incr     = a->incr;
    a->np.rate     = a->rate;
    a->np.msize    = a->msize;
    a->np.lambda_y = a->g.lambda_y;
    a->np.lambda_d = a->g.lambda_d;

    double tau;

    tau         = -128.0 / 8000.0 / log(0.7);
    a->np.alphaCsmooth = exp(-a->np.incr / a->np.rate / tau);

    tau     = -128.0 / 8000.0 / log(0.96);
    a->np.alphaMax = exp(-a->np.incr / a->np.rate / tau);

    tau      = -128.0 / 8000.0 / log(0.7);
    a->np.alphaCmin = exp(-a->np.incr / a->np.rate / tau);

    tau               = -128.0 / 8000.0 / log(0.3);
    a->np.alphaMin_max_value = exp(-a->np.incr / a->np.rate / tau);
    a->np.snrq = -a->np.incr / (0.064 * a->np.rate);

    tau    = -128.0 / 8000.0 / log(0.8);
    a->np.betamax = exp(-a->np.incr / a->np.rate / tau);

    a->np.invQeqMax = 0.5;
    a->np.av        = 2.12;
    a->np.Dtime     = 8.0 * 12.0 * 128.0 / 8000.0;
    a->np.U         = 8;
    a->np.V         = (int) (0.5 + (a->np.Dtime * a->np.rate / (a->np.U * a->np.incr)));

    if (a->np.V < 4) {
        a->np.V = 4;
    }

    if ((a->np.U = (int) (0.5 + (a->np.Dtime * a->np.rate / (a->np.V * a->np.incr)))) < 1) {
        a->np.U = 1;
    }

    a->np.D = a->np.U * a->np.V;
    interpM(&a->np.MofD, a->np.D, 18, Dvals, Mvals);
    interpM(&a->np.MofV, a->np.V, 18, Dvals, Mvals);
    a->np.invQbar_points[0] = 0.03;
    a->np.invQbar_points[1] = 0.05;
    a->np.invQbar_points[2] = 0.06;
    a->np.invQbar_points[3] = 1.0e300;

    double db;

    db             = 10.0 * log10(8.0) / (12.0 * 128 / 8000);
    a->np.nsmax[0] = pow(10.0, db / 10.0 * a->np.V * a->np.incr / a->np.rate);
    db             = 10.0 * log10(4.0) / (12.0 * 128 / 8000);
    a->np.nsmax[1] = pow(10.0, db / 10.0 * a->np.V * a->np.incr / a->np.rate);
    db             = 10.0 * log10(2.0) / (12.0 * 128 / 8000);
    a->np.nsmax[2] = pow(10.0, db / 10.0 * a->np.V * a->np.incr / a->np.rate);
    db             = 10.0 * log10(1.2) / (12.0 * 128 / 8000);
    a->np.nsmax[3] = pow(10.0, db / 10.0 * a->np.V * a->np.incr / a->np.rate);

    a->np.p           = (double *) calloc(a->np.msize, sizeof(double));
    a->np.alphaOptHat = (double *) calloc(a->np.msize, sizeof(double));
    a->np.alphaHat    = (double *) calloc(a->np.msize, sizeof(double));
    a->np.sigma2N     = (double *) calloc(a->np.msize, sizeof(double));
    a->np.pbar        = (double *) calloc(a->np.msize, sizeof(double));
    a->np.p2bar       = (double *) calloc(a->np.msize, sizeof(double));
    a->np.Qeq         = (double *) calloc(a->np.msize, sizeof(double));
    a->np.bmin        = (double *) calloc(a->np.msize, sizeof(double));
    a->np.bmin_sub    = (double *) calloc(a->np.msize, sizeof(double));
    a->np.k_mod       = (int *) calloc(a->np.msize, sizeof(int));
    a->np.actmin      = (double *) calloc(a->np.msize, sizeof(double));
    a->np.actmin_sub  = (double *) calloc(a->np.msize, sizeof(double));
    a->np.lmin_flag   = (int *) calloc(a->np.msize, sizeof(int));
    a->np.pmin_u      = (double *) calloc(a->np.msize, sizeof(double));
    a->np.actminbuff  = (double **) calloc(a->np.U, sizeof(double *));

    for (i = 0; i < a->np.U; i++) {
        a->np.actminbuff[i] = (double *) calloc(a->np.msize, sizeof(double));
    }

    {
        int k, ku;
        a->np.alphaC  = 1.0;
        a->np.subwc   = a->np.V;
        a->np.amb_idx = 0;

        for (k = 0; k < a->np.msize; k++) {
            a->np.lambda_y[k] = 0.5;
        }

        memcpy(a->np.p, a->np.lambda_y, a->np.msize * sizeof(double));
        memcpy(a->np.sigma2N, a->np.lambda_y, a->np.msize * sizeof(double));
        memcpy(a->np.pbar, a->np.lambda_y, a->np.msize * sizeof(double));
        memcpy(a->np.pmin_u, a->np.lambda_y, a->np.msize * sizeof(double));

        for (k = 0; k < a->np.msize; k++) {
            a->np.p2bar[k]      = a->np.lambda_y[k] * a->np.lambda_y[k];
            a->np.actmin[k]     = 1.0e300;
            a->np.actmin_sub[k] = 1.0e300;
            for (ku = 0; ku < a->np.U; ku++) {
                a->np.actminbuff[ku][k] = 1.0e300;
            }
        }
        memset(a->np.lmin_flag, 0, a->np.msize * sizeof(int));
    }

    /* nps */

    a->nps.incr     = a->incr;
    a->nps.rate     = a->rate;
    a->nps.msize    = a->msize;
    a->nps.lambda_y = a->g.lambda_y;
    a->nps.lambda_d = a->g.lambda_d;

    tau       = -128.0 / 8000.0 / log(0.8);
    a->nps.alpha_pow = exp(-a->nps.incr / a->nps.rate / tau);

    tau        = -128.0 / 8000.0 / log(0.9);
    a->nps.alpha_Pbar = exp(-a->nps.incr / a->nps.rate / tau);

    a->nps.epsH1   = pow(10.0, 15.0 / 10.0);
    a->nps.epsH1r  = a->nps.epsH1 / (1.0 + a->nps.epsH1);

    a->nps.sigma2N = (double *) calloc(a->nps.msize, sizeof(double));
    a->nps.PH1y    = (double *) calloc(a->nps.msize, sizeof(double));
    a->nps.Pbar    = (double *) calloc(a->nps.msize, sizeof(double));
    a->nps.EN2y    = (double *) calloc(a->nps.msize, sizeof(double));

    for (i = 0; i < a->nps.msize; i++) {
        a->nps.sigma2N[i] = 0.5;
        a->nps.Pbar[i]    = 0.5;
    }

    /* npl */

    a->npl.rate     = a->rate;
    a->npl.msize    = a->msize;
    a->npl.incr     = a->incr;
    a->npl.Ysq      = a->g.lambda_y;
    a->npl.P        = (double *) calloc(a->npl.msize, sizeof(double));
    a->npl.Pmin     = (double *) calloc(a->npl.msize, sizeof(double));
    a->npl.p        = (double *) calloc(a->npl.msize, sizeof(double));
    a->npl.D        = (double *) calloc(a->npl.msize, sizeof(double));
    a->npl.lambda_d = a->g.lambda_d;

    tau = -256.0 / (20100.0 * log(0.7));
    a->npl.eta = exp(-a->npl.incr / (a->npl.rate * tau));

    tau   = -256.0 / (20100.0 * log(0.998));
    a->npl.gamma = exp(-a->npl.incr / (a->npl.rate * tau));

    tau  = -256.0 / (20100.0 * log(0.8));
    a->npl.beta = exp(-a->npl.incr / (a->npl.rate * tau));

    tau     = -256.0 / (20100.0 * log(0.85));
    a->npl.alpha_d = exp(-a->npl.incr / (a->npl.rate * tau));

    tau     = -256.0 / (20100.0 * log(0.2));
    a->npl.alpha_p = exp(-a->npl.incr / (a->npl.rate * tau));

    a->npl.delta_LF = 1000.0 / (a->npl.rate / 2) * a->npl.msize;
    a->npl.delta_MF = 3000.0 / (a->npl.rate / 2) * a->npl.msize;
    a->npl.delta_0  = 2.0;
    a->npl.delta_1  = 2.0;
    a->npl.delta_2  = 5.0;

    /* ae */

    a->ae.msize      = a->msize;
    a->ae.lambda_y   = a->g.lambda_y;

    a->ae.zetaThresh = 0.75;
    a->ae.psi        = 20.0;
    a->ae.t2         = 0.20;
    a->ae.nmask      = (double *) calloc(a->ae.msize, sizeof(double));
}

void decalc_emnr(emnr_t *a) {
/*
    int i;
    _aligned_free(a->ae.nmask);
    // npl
    _aligned_free(a->npl.D);
    _aligned_free(a->npl.p);
    _aligned_free(a->npl.Pmin);
    _aligned_free(a->npl.P);
    // nps
    _aligned_free(a->nps.EN2y);
    _aligned_free(a->nps.Pbar);
    _aligned_free(a->nps.PH1y);
    _aligned_free(a->nps.sigma2N);
    // np
    for (i = 0; i < a->np.U; i++) {
        _aligned_free(a->np.actminbuff[i]);
    }
    _aligned_free(a->np.actminbuff);
    _aligned_free(a->np.pmin_u);
    _aligned_free(a->np.lmin_flag);
    _aligned_free(a->np.actmin_sub);
    _aligned_free(a->np.actmin);
    _aligned_free(a->np.k_mod);
    _aligned_free(a->np.bmin_sub);
    _aligned_free(a->np.bmin);
    _aligned_free(a->np.Qeq);
    _aligned_free(a->np.p2bar);
    _aligned_free(a->np.pbar);
    _aligned_free(a->np.sigma2N);
    _aligned_free(a->np.alphaHat);
    _aligned_free(a->np.alphaOptHat);
    _aligned_free(a->np.p);
    // g
    _aligned_free(a->g.zeta_true);
    _aligned_free(a->g.zeta_hat);
    _aligned_free(a->g.GGS);
    _aligned_free(a->g.GG);
    _aligned_free(a->g.prev_mask);
    _aligned_free(a->g.prev_gamma);
    _aligned_free(a->g.lambda_d);
    _aligned_free(a->g.lambda_y);
    //
    fftw_destroy_plan(a->Rrev);
    fftw_destroy_plan(a->Rfor);
    _aligned_free(a->outaccum);
    for (i = 0; i < a->ovrlp; i++) {
        _aligned_free(a->save[i]);
    }
    _aligned_free(a->save);
    _aligned_free(a->revfftout);
    _aligned_free(a->revfftin);
    _aligned_free(a->mask);
    _aligned_free(a->forfftout);
    _aligned_free(a->forfftin);
    _aligned_free(a->inaccum);
    _aligned_free(a->window);
*/
}

emnr_t * emnr_create(
    int size, float *in, float *out, int fsize, int ovrlp, int rate, int wintype,
    double gain, emnr_gain_t gain_method, emnr_npe_t npe_method, int ae_run) {

    emnr_t *a        = (emnr_t *) malloc(sizeof(emnr_t));

    a->bsize         = size;
    a->in            = in;
    a->out           = out;
    a->fsize         = fsize;
    a->ovrlp         = ovrlp;
    a->rate          = rate;
    a->wintype       = wintype;
    a->ogain         = gain;
    a->g.gain_method = gain_method;
    a->g.npe_method  = npe_method;
    a->g.ae_run      = ae_run;

    calc_emnr(a);

    return a;
}

void emnr_flush(emnr_t *a) {
    int i;

    memset(a->inaccum, 0, a->iasize * sizeof(float));

    for (i = 0; i < a->ovrlp; i++) {
        memset(a->save[i], 0, a->fsize * sizeof(double));
    }

    memset(a->outaccum, 0, a->oasize * sizeof(double));

    a->nsamps   = 0;
    a->iainidx  = 0;
    a->iaoutidx = 0;
    a->oainidx  = a->init_oainidx;
    a->oaoutidx = 0;
    a->saveidx  = 0;
}

void emnr_destroy(emnr_t *a) {
    decalc_emnr(a);
    free(a);
}

void LambdaD(emnr_t *a) {
    int    k;
    double f0, f1, f2, f3;
    double sum_prev_p;
    double sum_lambda_y;
    double alphaCtilda;
    double sum_prev_sigma2N;
    double alphaMin, SNR;
    double beta, varHat, invQeq;
    double invQbar;
    double bc;
    double QeqTilda, QeqTildaSub;
    double noise_slope_max;

    sum_prev_p       = 0.0;
    sum_lambda_y     = 0.0;
    sum_prev_sigma2N = 0.0;

    for (k = 0; k < a->np.msize; k++) {
        sum_prev_p += a->np.p[k];
        sum_lambda_y += a->np.lambda_y[k];
        sum_prev_sigma2N += a->np.sigma2N[k];
    }

    for (k = 0; k < a->np.msize; k++) {
        f0                   = a->np.p[k] / a->np.sigma2N[k] - 1.0;
        a->np.alphaOptHat[k] = 1.0 / (1.0 + f0 * f0);
    }

    SNR      = sum_prev_p / sum_prev_sigma2N;
    alphaMin = fmin(a->np.alphaMin_max_value, pow(SNR, a->np.snrq));

    for (k = 0; k < a->np.msize; k++) {
        if (a->np.alphaOptHat[k] < alphaMin) {
            a->np.alphaOptHat[k] = alphaMin;
        }
    }

    f1           = sum_prev_p / sum_lambda_y - 1.0;
    alphaCtilda  = 1.0 / (1.0 + f1 * f1);
    a->np.alphaC = a->np.alphaCsmooth * a->np.alphaC + (1.0 - a->np.alphaCsmooth) * fmax(alphaCtilda, a->np.alphaCmin);
    f2           = a->np.alphaMax * a->np.alphaC;

    for (k = 0; k < a->np.msize; k++) {
        a->np.alphaHat[k] = f2 * a->np.alphaOptHat[k];
    }

    for (k = 0; k < a->np.msize; k++) {
        a->np.p[k] = a->np.alphaHat[k] * a->np.p[k] + (1.0 - a->np.alphaHat[k]) * a->np.lambda_y[k];
    }

    invQbar = 0.0;

    for (k = 0; k < a->np.msize; k++) {
        beta           = fmin(a->np.betamax, a->np.alphaHat[k] * a->np.alphaHat[k]);
        a->np.pbar[k]  = beta * a->np.pbar[k] + (1.0 - beta) * a->np.p[k];
        a->np.p2bar[k] = beta * a->np.p2bar[k] + (1.0 - beta) * a->np.p[k] * a->np.p[k];
        varHat         = a->np.p2bar[k] - a->np.pbar[k] * a->np.pbar[k];
        invQeq         = varHat / (2.0 * a->np.sigma2N[k] * a->np.sigma2N[k]);

        if (invQeq > a->np.invQeqMax) {
            invQeq = a->np.invQeqMax;
        }

        a->np.Qeq[k] = 1.0 / invQeq;
        invQbar += invQeq;
    }

    invQbar /= (double) a->np.msize;
    bc = 1.0 + a->np.av * sqrt(invQbar);

    for (k = 0; k < a->np.msize; k++) {
        QeqTilda          = (a->np.Qeq[k] - 2.0 * a->np.MofD) / (1.0 - a->np.MofD);
        QeqTildaSub       = (a->np.Qeq[k] - 2.0 * a->np.MofV) / (1.0 - a->np.MofV);
        a->np.bmin[k]     = 1.0 + 2.0 * (a->np.D - 1.0) / QeqTilda;
        a->np.bmin_sub[k] = 1.0 + 2.0 * (a->np.V - 1.0) / QeqTildaSub;
    }

    memset(a->np.k_mod, 0, a->np.msize * sizeof(int));

    for (k = 0; k < a->np.msize; k++) {
        f3 = a->np.p[k] * a->np.bmin[k] * bc;
        if (f3 < a->np.actmin[k]) {
            a->np.actmin[k]     = f3;
            a->np.actmin_sub[k] = a->np.p[k] * a->np.bmin_sub[k] * bc;
            a->np.k_mod[k]      = 1;
        }
    }

    if (a->np.subwc == a->np.V) {
        if (invQbar < a->np.invQbar_points[0]) {
            noise_slope_max = a->np.nsmax[0];
        } else if (invQbar < a->np.invQbar_points[1]) {
            noise_slope_max = a->np.nsmax[1];
        } else if (invQbar < a->np.invQbar_points[2]) {
            noise_slope_max = a->np.nsmax[2];
        } else {
            noise_slope_max = a->np.nsmax[3];
        }

        for (k = 0; k < a->np.msize; k++) {
            int    ku;
            double min;

            if (a->np.k_mod[k]) {
                a->np.lmin_flag[k] = 0;
            }

            a->np.actminbuff[a->np.amb_idx][k] = a->np.actmin[k];
            min                                = 1.0e300;

            for (ku = 0; ku < a->np.U; ku++) {
                if (a->np.actminbuff[ku][k] < min) {
                    min = a->np.actminbuff[ku][k];
                }
            }

            a->np.pmin_u[k] = min;

            if ((a->np.lmin_flag[k] == 1) && (a->np.actmin_sub[k] < noise_slope_max * a->np.pmin_u[k]) &&
                (a->np.actmin_sub[k] > a->np.pmin_u[k])) {
                a->np.pmin_u[k] = a->np.actmin_sub[k];
                for (ku = 0; ku < a->np.U; ku++) {
                    a->np.actminbuff[ku][k] = a->np.actmin_sub[k];
                }
            }

            a->np.lmin_flag[k]  = 0;
            a->np.actmin[k]     = 1.0e300;
            a->np.actmin_sub[k] = 1.0e300;
        }

        if (++a->np.amb_idx == a->np.U) {
            a->np.amb_idx = 0;
        }

        a->np.subwc = 1;
    } else {
        if (a->np.subwc > 1) {
            for (k = 0; k < a->np.msize; k++) {
                if (a->np.k_mod[k]) {
                    a->np.lmin_flag[k] = 1;
                    a->np.sigma2N[k]   = fmin(a->np.actmin_sub[k], a->np.pmin_u[k]);
                    a->np.pmin_u[k]    = a->np.sigma2N[k];
                }
            }
        }
        ++a->np.subwc;
    }
    memcpy(a->np.lambda_d, a->np.sigma2N, a->np.msize * sizeof(double));
}

void LambdaDs(emnr_t *a) {
    int k;

    for (k = 0; k < a->nps.msize; k++) {
        a->nps.PH1y[k] =
            1.0 / (1.0 + (1.0 + a->nps.epsH1) * exp(-a->nps.epsH1r * a->nps.lambda_y[k] / a->nps.sigma2N[k]));
        a->nps.Pbar[k] = a->nps.alpha_Pbar * a->nps.Pbar[k] + (1.0 - a->nps.alpha_Pbar) * a->nps.PH1y[k];

        if (a->nps.Pbar[k] > 0.99) {
            a->nps.PH1y[k] = fmin(a->nps.PH1y[k], 0.99);
        }

        a->nps.EN2y[k]    = (1.0 - a->nps.PH1y[k]) * a->nps.lambda_y[k] + a->nps.PH1y[k] * a->nps.sigma2N[k];
        a->nps.sigma2N[k] = a->nps.alpha_pow * a->nps.sigma2N[k] + (1.0 - a->nps.alpha_pow) * a->nps.EN2y[k];
    }

    memcpy(a->nps.lambda_d, a->nps.sigma2N, a->nps.msize * sizeof(double));
}

void LambdaDl(emnr_t *a) {
    double P_old, c, Sr, delta, I, alpha_s;

    c = (1.0 - a->npl.gamma) / (1.0 - a->npl.beta);

    for (int k = 0; k < a->npl.msize; k++) {
        P_old       = a->npl.P[k];
        a->npl.P[k] = a->npl.eta * P_old + (1.0 - a->npl.eta) * a->npl.Ysq[k];

        if (a->npl.Pmin[k] < a->npl.P[k]) {
            a->npl.Pmin[k] = a->npl.gamma * a->npl.Pmin[k] + c * (a->npl.P[k] - a->npl.beta * P_old);
        } else {
            a->npl.Pmin[k] = a->npl.P[k];
        }

        Sr = a->npl.P[k] / a->npl.Pmin[k];

        if (k <= a->npl.delta_LF) {
            delta = a->npl.delta_0;
        } else if (k <= a->npl.delta_MF) {
            delta = a->npl.delta_1;
        } else {
            delta = a->npl.delta_2;
        }

        if (Sr > delta) {
            I = 1.0;
        } else {
            I = 0.0;
        }

        a->npl.p[k] = a->npl.alpha_p * a->npl.p[k] + (1.0 - a->npl.alpha_p) * I;
        alpha_s     = a->npl.alpha_d + (1.0 - a->npl.alpha_d) * a->npl.p[k];
        a->npl.D[k] = alpha_s * a->npl.D[k] + (1.0 - alpha_s) * a->npl.Ysq[k];
    }

    memcpy(a->npl.lambda_d, a->npl.D, a->npl.msize * sizeof(double));
}

void aepf(emnr_t *a) {
    int    k, m;
    int    N, n;
    double sumPre, sumPost, zeta, zetaT;

    sumPre  = 0.0;
    sumPost = 0.0;

    for (k = 0; k < a->ae.msize; k++) {
        sumPre += a->ae.lambda_y[k];
        sumPost += a->mask[k] * a->mask[k] * a->ae.lambda_y[k];
    }

    zeta = sumPost / sumPre;

    if (zeta >= a->ae.zetaThresh) {
        zetaT = 1.0;
    } else {
        zetaT = zeta;
    }

    if (zetaT == 1.0) {
        N = 1;
    } else {
        N = 1 + 2 * (int) (0.5 + a->ae.psi * (1.0 - zetaT / a->ae.zetaThresh));
    }

    n = N / 2;

    for (k = 0; k < n; k++) {
        a->ae.nmask[k] = 0.0;

        for (m = 0; m <= 2 * k; m++) {
            a->ae.nmask[k] += a->mask[m];
        }

        a->ae.nmask[k] /= (double) (2 * k + 1);
    }

    for (k = n; k < (a->ae.msize - n); k++) {
        a->ae.nmask[k] = 0.0;

        for (m = k - n; m <= (k + n); m++) {
            a->ae.nmask[k] += a->mask[m];
        }

        a->ae.nmask[k] /= (double) N;
    }

    for (k = a->ae.msize - n; k < a->ae.msize; k++) {
        a->ae.nmask[k] = 0.0;

        for (m = (a->ae.msize - 1); m >= (-a->ae.msize + 2 * k + 1); m--) {
            a->ae.nmask[k] += a->mask[m];
        }

        a->ae.nmask[k] /= (double) (2 * (a->ae.msize - k) - 1);
    }

    memcpy(a->mask, a->ae.nmask, a->ae.msize * sizeof(double));

    if (a->g.gain_method == EMNR_GAIN_TRAINED && zetaT < a->ae.t2) {
        for (k = 0; k < a->ae.msize; k++) {
            a->mask[k] *= 0.05;
        }
    }
}

double getKey(double *type, double gamma, double xi) {
    int          ngamma1, ngamma2, nxi1, nxi2;
    double       tg, tx, dg, dx;
    const double dmin = 0.001;
    const double dmax = 1000.0;

    if (gamma <= dmin) {
        ngamma1 = ngamma2 = 0;
        tg                = 0.0;
    } else if (gamma >= dmax) {
        ngamma1 = ngamma2 = 240;
        tg                = 60.0;
    } else {
        tg      = 10.0 * log10(gamma / dmin);
        ngamma1 = (int) (4.0 * tg);
        ngamma2 = ngamma1 + 1;
    }

    if (xi <= dmin) {
        nxi1 = nxi2 = 0;
        tx          = 0.0;
    } else if (xi >= dmax) {
        nxi1 = nxi2 = 240;
        tx          = 60.0;
    } else {
        tx   = 10.0 * log10(xi / dmin);
        nxi1 = (int) (4.0 * tx);
        nxi2 = nxi1 + 1;
    }

    dg = (tg - 0.25 * ngamma1) / 0.25;
    dx = (tx - 0.25 * nxi1) / 0.25;

    return (1.0 - dg) * (1.0 - dx) * type[241 * nxi1 + ngamma1] + (1.0 - dg) * dx * type[241 * nxi2 + ngamma1] +
           dg * (1.0 - dx) * type[241 * nxi1 + ngamma2] + dg * dx * type[241 * nxi2 + ngamma2];
}

void calc_gain(emnr_t *a) {
    int k;

    for (k = 0; k < a->g.msize; k++) {
        a->g.lambda_y[k] = a->g.y[2 * k + 0] * a->g.y[2 * k + 0] + a->g.y[2 * k + 1] * a->g.y[2 * k + 1];
    }

    switch (a->g.npe_method) {
        case EMNR_NPE_OSMS:
            LambdaD(a);
            break;
        case EMNR_NPE_MMSE:
            LambdaDs(a);
            break;
        case EMNR_NPE_NSTAT:
            LambdaDl(a);
            break;
    }

    switch (a->g.gain_method) {
        case EMNR_GAIN_GAUSIAN_LINEAR: {
            double gamma, eps_hat, v;

            for (k = 0; k < a->g.msize; k++) {
                gamma   = fmin(a->g.lambda_y[k] / a->g.lambda_d[k], a->g.gamma_max);
                eps_hat = a->g.alpha * a->g.prev_mask[k] * a->g.prev_mask[k] * a->g.prev_gamma[k] +
                          (1.0 - a->g.alpha) * fmax(gamma - 1.0, a->g.eps_floor);
                eps_hat = fmax(eps_hat, a->g.xi_min);
                v       = (eps_hat / (1.0 + eps_hat)) * gamma;
                a->g.mask[k] =
                    a->g.gf1p5 * sqrt(v) / gamma * exp(-0.5 * v) * ((1.0 + v) * bessI0(0.5 * v) + v * bessI1(0.5 * v));
                {
                    double v2       = fmin(v, 700.0);
                    double eta      = a->g.mask[k] * a->g.mask[k] * a->g.lambda_y[k] / a->g.lambda_d[k];
                    double eps      = eta / (1.0 - a->g.q);
                    double witchHat = (1.0 - a->g.q) / a->g.q * exp(v2) / (1.0 + eps);

                    a->g.mask[k] *= witchHat / (1.0 + witchHat);
                }

                if (a->g.mask[k] > a->g.gmax) {
                    a->g.mask[k] = a->g.gmax;
                }

                if (a->g.mask[k] != a->g.mask[k]) {
                    a->g.mask[k] = 0.01;
                }

                a->g.prev_gamma[k] = gamma;
                a->g.prev_mask[k]  = a->g.mask[k];
            }
            break;
        }

        case EMNR_GAIN_GAUSIAN_LOG: {
            double gamma, eps_hat, v, ehr;

            for (k = 0; k < a->g.msize; k++) {
                gamma   = fmin(a->g.lambda_y[k] / a->g.lambda_d[k], a->g.gamma_max);
                eps_hat = a->g.alpha * a->g.prev_mask[k] * a->g.prev_mask[k] * a->g.prev_gamma[k] +
                          (1.0 - a->g.alpha) * fmax(gamma - 1.0, a->g.eps_floor);
                ehr = eps_hat / (1.0 + eps_hat);
                v   = ehr * gamma;

                if ((a->g.mask[k] = ehr * exp(fmin(700.0, 0.5 * e1xb(v)))) > a->g.gmax) {
                    a->g.mask[k] = a->g.gmax;
                }

                if (a->g.mask[k] != a->g.mask[k]) {
                    a->g.mask[k] = 0.01;
                }

                a->g.prev_gamma[k] = gamma;
                a->g.prev_mask[k]  = a->g.mask[k];
            }
            break;
        }

        case EMNR_GAIN_GAMMA: {
            double gamma, eps_hat, eps_p;

            for (k = 0; k < a->g.msize; k++) {
                gamma   = fmin(a->g.lambda_y[k] / a->g.lambda_d[k], a->g.gamma_max);
                eps_hat = a->g.alpha * a->g.prev_mask[k] * a->g.prev_mask[k] * a->g.prev_gamma[k] +
                          (1.0 - a->g.alpha) * fmax(gamma - 1.0, a->g.eps_floor);
                eps_p              = eps_hat / (1.0 - a->g.q);
                a->g.mask[k]       = getKey(a->g.GG, gamma, eps_hat) * getKey(a->g.GGS, gamma, eps_p);
                a->g.prev_gamma[k] = gamma;
                a->g.prev_mask[k]  = a->g.mask[k];
            }
            break;
        }

        case EMNR_GAIN_TRAINED: {
            double gamma, xi_hat, v, zeta_hat;

            for (k = 0; k < a->g.msize; k++) {
                gamma  = fmin(a->g.lambda_y[k] / a->g.lambda_d[k], a->g.gamma_max);
                xi_hat = a->g.alpha * a->g.prev_mask[k] * a->g.prev_mask[k] * a->g.prev_gamma[k] +
                         (1.0 - a->g.alpha) * fmax(gamma - 1.0, a->g.eps_floor);
                xi_hat = fmax(xi_hat, a->g.xi_min);
                v      = (xi_hat / (1.0 + xi_hat)) * gamma;
                a->g.mask[k] =
                    a->g.gf1p5 * sqrt(v) / gamma * exp(-0.5 * v) * ((1.0 + v) * bessI0(0.5 * v) + v * bessI1(0.5 * v));
                {
                    double v2       = fmin(v, 700.0);
                    double eta      = a->g.mask[k] * a->g.mask[k] * a->g.lambda_y[k] / a->g.lambda_d[k];
                    double eps      = eta / (1.0 - a->g.q);
                    double witchHat = (1.0 - a->g.q) / a->g.q * exp(v2) / (1.0 + eps);
                    a->g.mask[k] *= witchHat / (1.0 + witchHat);
                }

                if (a->g.mask[k] > a->g.gmax) {
                    a->g.mask[k] = a->g.gmax;
                }

                if (a->g.mask[k] != a->g.mask[k]) {
                    a->g.mask[k] = 0.01;
                }

                a->g.prev_mask[k]  = a->g.mask[k];
                a->g.prev_gamma[k] = gamma;

                {
                    double xi_ts = a->g.mask[k] * a->g.mask[k] * gamma;

                    xi_ts        = fmax(xi_ts, a->g.xi_min);

                    double v_ts  = (xi_ts / (1.0 + xi_ts)) * gamma;

                    a->g.mask[k] = a->g.gf1p5 * sqrt(v_ts) / gamma * exp(-0.5 * v_ts) *
                                   ((1.0 + v_ts) * bessI0(0.5 * v_ts) + v_ts * bessI1(0.5 * v_ts));

                    double v2       = fmin(v, 700.0);
                    double eta      = a->g.mask[k] * a->g.mask[k] * a->g.lambda_y[k] / a->g.lambda_d[k];
                    double eps      = eta / (1.0 - a->g.q);
                    double witchHat = (1.0 - a->g.q) / a->g.q * exp(v2) / (1.0 + eps);

                    a->g.mask[k] *= witchHat / (1.0 + witchHat);
                    xi_hat = xi_ts;
                }

                if (get_zeta(gamma, xi_hat, &zeta_hat) >= 0) {
                    if (zeta_hat > a->g.zeta_thresh) {
                        a->g.mask[k] = 1.0;
                    } else {
                        a->g.mask[k] = 0.0;
                    }
                }
            }
            break;
        }
    }

    if (a->g.ae_run) {
        aepf(a);
    }
}

void emnr_apply(emnr_t *a) {
    int    i, j, k, sbuff, sbegin;
    double g1;

    for (i = 0; i < a->bsize; i++) {
        a->inaccum[a->iainidx] = a->in[i];
        a->iainidx             = (a->iainidx + 1) % a->iasize;
    }

    a->nsamps += a->bsize;

    while (a->nsamps >= a->fsize) {
        for (i = 0, j = a->iaoutidx; i < a->fsize; i++, j = (j + 1) % a->iasize) {
            a->forfftin[i] = a->window[i] * a->inaccum[j];
        }

        a->iaoutidx = (a->iaoutidx + a->incr) % a->iasize;
        a->nsamps -= a->incr;
        fftw_execute(a->Rfor);
        calc_gain(a);

        for (i = 0; i < a->msize; i++) {
            g1                     = a->gain * a->mask[i];
            a->revfftin[2 * i + 0] = g1 * a->forfftout[2 * i + 0];
            a->revfftin[2 * i + 1] = g1 * a->forfftout[2 * i + 1];
        }

        fftw_execute(a->Rrev);

        for (i = 0; i < a->fsize; i++) {
            a->save[a->saveidx][i] = a->window[i] * a->revfftout[i];
        }

        for (i = a->ovrlp; i > 0; i--) {
            sbuff  = (a->saveidx + i) % a->ovrlp;
            sbegin = a->incr * (a->ovrlp - i);

            for (j = sbegin, k = a->oainidx; j < a->incr + sbegin; j++, k = (k + 1) % a->oasize) {
                if (i == a->ovrlp) {
                    a->outaccum[k] = a->save[sbuff][j];
                } else {
                    a->outaccum[k] += a->save[sbuff][j];
                }
            }
        }

        a->saveidx = (a->saveidx + 1) % a->ovrlp;
        a->oainidx = (a->oainidx + a->incr) % a->oasize;
    }

    for (i = 0; i < a->bsize; i++) {
        a->out[i] = a->outaccum[a->oaoutidx];
        a->oaoutidx = (a->oaoutidx + 1) % a->oasize;
    }
}

void emnr_set_gain_method(emnr_t *a, emnr_gain_t x) {
    a->g.gain_method = x;
}

void emnr_set_npe_method(emnr_t *a, emnr_npe_t x) {
    a->g.npe_method = x;
}

void emnr_set_trained_thresh(emnr_t *a, float x) {
    a->g.zeta_thresh = x;
}

void emnr_set_trained_t2(emnr_t *a, float x) {
    a->ae.t2 = x;
}
