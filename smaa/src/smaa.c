#include <R.h>
#include <R_ext/BLAS.h>

#include <math.h>
#include <stdlib.h>

#include <stdio.h>

typedef struct Matrix {
    double * const data;
    int const nRow;
    int const nCol;
} Matrix;

/**
 * @param i Row index.
 * @param j Column index.
 */
inline double *get(Matrix *m, int i, int j) {
    return m->data + j * (m->nRow) + i;
}

/**
 * Rank the n-array of doubles t, writing results to r.
 * The rank of t[i] is the number of elements greater than t[i].
 * Assumes that n is small (complexity O(n^2)).
 */
void rank(double *t, int *r, int n) {
	for (int i = 0; i < n; ++i) {
		r[i] = 0;
		for (int j = 0; j < n; ++j) {
			if (t[j] > t[i]) {
				++r[i];
			}
		}
	}
}

void smaa(
		double const *meas, double const *pref,
		int const *nIter, int const *nAlt, int const *nCrit,
		double *hData, double *wcData, double *tData) {
	const int inc1 = 1;
	const double one = 1.0, zero = 0.0; // for BLAS
	const char trans = 'N';

	Matrix h = { hData, *nAlt, *nAlt };
	Matrix wc = { wcData, *nAlt, *nCrit };

	double t_[*nAlt];
	double *t = (tData != 0 ? tData : t_); // alternative values
	int r[*nAlt]; // alternative ranks
	for (int k = 0; k < *nIter; ++k) {
		// calculate value of each alternative
		F77_CALL(dgemv)(&trans, nAlt, nCrit,
			&one, meas, nAlt, pref, &inc1,
			&zero, t, &inc1); // t := 1Aw + 0t

		// rank the alternatives
		rank(t, r, *nAlt);
		for (int i = 0; i < *nAlt; ++i) {
			*get(&h, i, r[i]) = *get(&h, i, r[i]) + 1; // update rank counts
			if (r[i] == 0) { // update central weights
				for (int j = 0; j < *nCrit; ++j) {
					*get(&wc, i, j) = *get(&wc, i, j) + pref[j];
				}
			}
		}

		// advance measurement and weight pointers
		meas += *nAlt * *nCrit;
		pref += *nCrit;
		// advance alternative value pointer
		if (tData != 0) {
			t += *nAlt;
		}
	}

	// normalize central weights
	for (int i = 0; i < *nAlt; ++i) {
		double const r1 = *get(&h, i, 0);
		if (r1 > 0.0) {
			for (int j = 0; j < *nCrit; ++j) {
				*get(&wc, i, j) = *get(&wc, i, j) / r1;
			}
		}
	}
}
