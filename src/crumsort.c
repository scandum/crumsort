// crumsort 1.2.1.3 - Igor van den Hoven ivdhoven@gmail.com

#define CRUM_AUX  512
#define CRUM_OUT   96

void FUNC(fulcrum_partition)(VAR *array, VAR *swap, VAR *max, size_t swap_size, size_t nmemb, CMPFUNC *cmp);

void FUNC(crum_analyze)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	unsigned char loop, asum, bsum, csum, dsum;
	unsigned int astreaks, bstreaks, cstreaks, dstreaks;
	size_t quad1, quad2, quad3, quad4, half1, half2;
	size_t cnt, abalance, bbalance, cbalance, dbalance;
	VAR *pta, *ptb, *ptc, *ptd;

	half1 = nmemb / 2;
	quad1 = half1 / 2;
	quad2 = half1 - quad1;
	half2 = nmemb - half1;
	quad3 = half2 / 2;
	quad4 = half2 - quad3;

	pta = array;
	ptb = array + quad1;
	ptc = array + half1;
	ptd = array + half1 + quad3;

	astreaks = bstreaks = cstreaks = dstreaks = 0;
	abalance = bbalance = cbalance = dbalance = 0;

	for (cnt = nmemb ; cnt > 132 ; cnt -= 128)
	{
		for (asum = bsum = csum = dsum = 0, loop = 32 ; loop ; loop--)
		{
			asum += cmp(pta, pta + 1) > 0; pta++;
			bsum += cmp(ptb, ptb + 1) > 0; ptb++;
			csum += cmp(ptc, ptc + 1) > 0; ptc++;
			dsum += cmp(ptd, ptd + 1) > 0; ptd++;
		}
		abalance += asum; astreaks += asum = (asum == 0) | (asum == 32);
		bbalance += bsum; bstreaks += bsum = (bsum == 0) | (bsum == 32);
		cbalance += csum; cstreaks += csum = (csum == 0) | (csum == 32);
		dbalance += dsum; dstreaks += dsum = (dsum == 0) | (dsum == 32);

		if (cnt > 516 && asum + bsum + csum + dsum == 0)
		{
			abalance += 48; pta += 96;
			bbalance += 48; ptb += 96;
			cbalance += 48; ptc += 96;
			dbalance += 48; ptd += 96;
			cnt -= 384;
		}
	}

	for ( ; cnt > 7 ; cnt -= 4)
	{
		abalance += cmp(pta, pta + 1) > 0; pta++;
		bbalance += cmp(ptb, ptb + 1) > 0; ptb++;
		cbalance += cmp(ptc, ptc + 1) > 0; ptc++;
		dbalance += cmp(ptd, ptd + 1) > 0; ptd++;
	}

	if (quad1 < quad2) {bbalance += cmp(ptb, ptb + 1) > 0; ptb++;}
	if (quad1 < quad3) {cbalance += cmp(ptc, ptc + 1) > 0; ptc++;}
	if (quad1 < quad4) {dbalance += cmp(ptd, ptd + 1) > 0; ptd++;}

	cnt = abalance + bbalance + cbalance + dbalance;

	if (cnt == 0)
	{
		if (cmp(pta, pta + 1) <= 0 && cmp(ptb, ptb + 1) <= 0 && cmp(ptc, ptc + 1) <= 0)
		{
			return;
		}
	}

	asum = quad1 - abalance == 1;
	bsum = quad2 - bbalance == 1;
	csum = quad3 - cbalance == 1;
	dsum = quad4 - dbalance == 1;

	if (asum | bsum | csum | dsum)
	{
		unsigned char span1 = (asum && bsum) * (cmp(pta, pta + 1) > 0);
		unsigned char span2 = (bsum && csum) * (cmp(ptb, ptb + 1) > 0);
		unsigned char span3 = (csum && dsum) * (cmp(ptc, ptc + 1) > 0);

		switch (span1 | span2 * 2 | span3 * 4)
		{
			case 0: break;
			case 1: FUNC(quad_reversal)(array, ptb);   abalance = bbalance = 0; break;
			case 2: FUNC(quad_reversal)(pta + 1, ptc); bbalance = cbalance = 0; break;
			case 3: FUNC(quad_reversal)(array, ptc);   abalance = bbalance = cbalance = 0; break;
			case 4: FUNC(quad_reversal)(ptb + 1, ptd); cbalance = dbalance = 0; break;
			case 5: FUNC(quad_reversal)(array, ptb);
				FUNC(quad_reversal)(ptb + 1, ptd); abalance = bbalance = cbalance = dbalance = 0; break;
			case 6: FUNC(quad_reversal)(pta + 1, ptd); bbalance = cbalance = dbalance = 0; break;
			case 7: FUNC(quad_reversal)(array, ptd); return;
		}

		if (asum && abalance) {FUNC(quad_reversal)(array,   pta); abalance = 0;}
		if (bsum && bbalance) {FUNC(quad_reversal)(pta + 1, ptb); bbalance = 0;}
		if (csum && cbalance) {FUNC(quad_reversal)(ptb + 1, ptc); cbalance = 0;}
		if (dsum && dbalance) {FUNC(quad_reversal)(ptc + 1, ptd); dbalance = 0;}
	}

#ifdef cmp
	cnt = nmemb / 256; // switch to quadsort if at least 50% ordered
#else
	cnt = nmemb / 512; // switch to quadsort if at least 25% ordered
#endif
	asum = astreaks > cnt;
	bsum = bstreaks > cnt;
	csum = cstreaks > cnt;
	dsum = dstreaks > cnt;

#ifndef cmp
	if (quad1 > QUAD_CACHE)
	{
//		asum = bsum = csum = dsum = 1;
		goto quad_cache;
	}
#endif

	switch (asum + bsum * 2 + csum * 4 + dsum * 8)
	{
		case 0:
			FUNC(fulcrum_partition)(array, swap, NULL, swap_size, nmemb, cmp);
			return;
		case 1:
			if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			FUNC(fulcrum_partition)(pta + 1, swap, NULL, swap_size, quad2 + half2, cmp);
			break;
		case 2:
			FUNC(fulcrum_partition)(array, swap, NULL, swap_size, quad1, cmp);
			if (bbalance) FUNC(quadsort_swap)(pta + 1, swap, swap_size, quad2, cmp);
			FUNC(fulcrum_partition)(ptb + 1, swap, NULL, swap_size, half2, cmp);
			break;
		case 3:
			if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			if (bbalance) FUNC(quadsort_swap)(pta + 1, swap, swap_size, quad2, cmp);
			FUNC(fulcrum_partition)(ptb + 1, swap, NULL, swap_size, half2, cmp);
			break;
		case 4:
			FUNC(fulcrum_partition)(array, swap, NULL, swap_size, half1, cmp);
			if (cbalance) FUNC(quadsort_swap)(ptb + 1, swap, swap_size, quad3, cmp);
			FUNC(fulcrum_partition)(ptc + 1, swap, NULL, swap_size, quad4, cmp);
			break;
		case 8:
			FUNC(fulcrum_partition)(array, swap, NULL, swap_size, half1 + quad3, cmp);
			if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 9:
			if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			FUNC(fulcrum_partition)(pta + 1, swap, NULL, swap_size, quad2 + quad3, cmp);
			if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 12:
			FUNC(fulcrum_partition)(array, swap, NULL, swap_size, half1, cmp);
			if (cbalance) FUNC(quadsort_swap)(ptb + 1, swap, swap_size, quad3, cmp);
			if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 5:
		case 6:
		case 7:
		case 10:
		case 11:
		case 13:
		case 14:
		case 15:
#ifndef cmp
		quad_cache:
#endif
			if (asum)
			{
				if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			}
			else FUNC(fulcrum_partition)(array, swap, NULL, swap_size, quad1, cmp);
			if (bsum)
			{
				if (bbalance) FUNC(quadsort_swap)(pta + 1, swap, swap_size, quad2, cmp);
			}
			else FUNC(fulcrum_partition)(pta + 1, swap, NULL, swap_size, quad2, cmp);
			if (csum)
			{
				if (cbalance) FUNC(quadsort_swap)(ptb + 1, swap, swap_size, quad3, cmp);
			}
			else FUNC(fulcrum_partition)(ptb + 1, swap, NULL, swap_size, quad3, cmp);
			if (dsum)
			{
				if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			}
			else FUNC(fulcrum_partition)(ptc + 1, swap, NULL, swap_size, quad4, cmp);
			break;
	}

	if (cmp(pta, pta + 1) <= 0)
	{
		if (cmp(ptc, ptc + 1) <= 0)
		{
			if (cmp(ptb, ptb + 1) <= 0)
			{
				return;
			}
		}
		else
		{
			FUNC(rotate_merge_block)(array + half1, swap, swap_size, quad3, quad4, cmp);
		}
	}
	else
	{
		FUNC(rotate_merge_block)(array, swap, swap_size, quad1, quad2, cmp);

		if (cmp(ptc, ptc + 1) > 0)
		{
			FUNC(rotate_merge_block)(array + half1, swap, swap_size, quad3, quad4, cmp);
		}
	}
	FUNC(rotate_merge_block)(array, swap, swap_size, half1, half2, cmp);
}

// The next 4 functions are used for pivot selection

VAR *FUNC(crum_binary_median)(VAR *pta, VAR *ptb, size_t len, CMPFUNC *cmp)
{
	while (len /= 2)
	{
		if (cmp(pta + len, ptb + len) <= 0) pta += len; else ptb += len;
	}
	return cmp(pta, ptb) > 0 ? pta : ptb;
}

VAR *FUNC(crum_median_of_cbrt)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, int *generic, CMPFUNC *cmp)
{
	VAR *pta, *piv;
	size_t cnt, cbrt, div;

	for (cbrt = 32 ; nmemb > cbrt * cbrt * cbrt && cbrt < swap_size ; cbrt *= 2) {}

	div = nmemb / cbrt;

	pta = array + nmemb - 1 - (size_t) &div / 64 % div;
	piv = array + cbrt;

	for (cnt = cbrt ; cnt ; cnt--)
	{
		swap[0] = *--piv; *piv = *pta; *pta = swap[0];

		pta -= div;
	}

	cbrt /= 2;

	FUNC(quadsort_swap)(piv, swap, swap_size, cbrt, cmp);
	FUNC(quadsort_swap)(piv + cbrt, swap, swap_size, cbrt, cmp);

	*generic = (cmp(piv + cbrt * 2 - 1, piv) <= 0) & (cmp(piv + cbrt - 1, piv) <= 0);

	return FUNC(crum_binary_median)(piv, piv + cbrt, cbrt, cmp);
}

size_t FUNC(crum_median_of_three)(VAR *array, size_t v0, size_t v1, size_t v2, CMPFUNC *cmp)
{
	size_t v[3] = {v0, v1, v2};
	char x, y, z;

	x = cmp(array + v0, array + v1) > 0;
	y = cmp(array + v0, array + v2) > 0;
	z = cmp(array + v1, array + v2) > 0;

	return v[(x == y) + (y ^ z)];
}

VAR *FUNC(crum_median_of_nine)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t x, y, z, div = nmemb / 16;

	x = FUNC(crum_median_of_three)(array, div * 2, div * 1, div * 4, cmp);
	y = FUNC(crum_median_of_three)(array, div * 8, div * 6, div * 10, cmp);
	z = FUNC(crum_median_of_three)(array, div * 14, div * 12, div * 15, cmp);

	return array + FUNC(crum_median_of_three)(array, x, y, z, cmp);
}

size_t FUNC(fulcrum_default_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *piv, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	size_t i, cnt, val, m = 0;
	VAR *ptl, *ptr, *pta, *tpa;

	memcpy(swap, array, 32 * sizeof(VAR));
	memcpy(swap + 32, array + nmemb - 32, 32 * sizeof(VAR));

	ptl = array;
	ptr = array + nmemb - 1;

	pta = array + 32;
	tpa = array + nmemb - 33;

	cnt = nmemb / 16 - 4;

	while (1)
	{
		if (pta - ptl - m <= 48)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
			}
		}
		if (pta - ptl - m >= 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(tpa, piv) <= 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
			}
		}
	}

	if (pta - ptl - m <= 48)
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		}
	}
	else
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(tpa, piv) <= 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
		}
	}
	pta = swap;

	for (cnt = 16 ; cnt ; cnt--)
	{
		val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
	}
	return m;
}

// As per suggestion by Marshall Lochbaum to improve generic data handling by mimicking dual-pivot quicksort

size_t FUNC(fulcrum_reverse_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *piv, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	size_t i, cnt, val, m = 0;
	VAR *ptl, *ptr, *pta, *tpa;

	memcpy(swap, array, 32 * sizeof(VAR));
	memcpy(swap + 32, array + nmemb - 32, 32 * sizeof(VAR));

	ptl = array;
	ptr = array + nmemb - 1;

	pta = array + 32;
	tpa = array + nmemb - 33;

	cnt = nmemb / 16 - 4;

	while (1)
	{
		if (pta - ptl - m <= 48)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
			}
		}
		if (pta - ptl - m >= 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(piv, tpa) > 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
			}
		}
	}

	if (pta - ptl - m <= 48)
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		}
	}
	else
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(piv, tpa) > 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
		}
	}
	pta = swap;

	for (cnt = 16 ; cnt ; cnt--)
	{
		val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
	}
	return m;
}

void FUNC(fulcrum_partition)(VAR *array, VAR *swap, VAR *max, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	size_t a_size, s_size;
	VAR *ptp, piv;
	int generic = 0;

	while (1)
	{
		if (nmemb <= 2048)
		{
			ptp = FUNC(crum_median_of_nine)(array, nmemb, cmp);
		}
		else
		{
			ptp = FUNC(crum_median_of_cbrt)(array, swap, swap_size, nmemb, &generic, cmp);

			if (generic) break;
		}
		piv = *ptp;

		if (max && cmp(max, &piv) <= 0)
		{
			a_size = FUNC(fulcrum_reverse_partition)(array, swap, array, &piv, swap_size, nmemb, cmp);
			s_size = nmemb - a_size;
			nmemb = a_size;

			if (s_size <= a_size / 32 || a_size <= CRUM_OUT) break;

			max = NULL;
			continue;
		}
		*ptp = array[--nmemb];

		a_size = FUNC(fulcrum_default_partition)(array, swap, array, &piv, swap_size, nmemb, cmp);
		s_size = nmemb - a_size;

		ptp = array + a_size; array[nmemb] = *ptp; *ptp = piv;

		if (a_size <= s_size / 32 || s_size <= CRUM_OUT)
		{
			FUNC(quadsort_swap)(ptp + 1, swap, swap_size, s_size, cmp);
		}
		else
		{
			FUNC(fulcrum_partition)(ptp + 1, swap, max, swap_size, s_size, cmp);
		}
		nmemb = a_size;

		if (s_size <= a_size / 32 || a_size <= CRUM_OUT)
		{
			if (a_size <= CRUM_OUT) break;

			a_size = FUNC(fulcrum_reverse_partition)(array, swap, array, &piv, swap_size, nmemb, cmp);
			s_size = nmemb - a_size;
			nmemb = a_size;

			if (s_size <= a_size / 32 || a_size <= CRUM_OUT) break;

			max = NULL;
			continue;
		}
		max = ptp;
	}
	FUNC(quadsort_swap)(array, swap, swap_size, nmemb, cmp);
}

void FUNC(crumsort)(void *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb <= 256)
	{
		VAR swap[nmemb];

		FUNC(quadsort_swap)(array, swap, nmemb, nmemb, cmp);

		return;
	}
	VAR *pta = (VAR *) array;
#if CRUM_AUX
	size_t swap_size = CRUM_AUX;
#else
	size_t swap_size = 128;

	while (swap_size * swap_size <= nmemb)
	{
		swap_size *= 4;
	}
#endif
	VAR swap[swap_size];

	FUNC(crum_analyze)(pta, swap, swap_size, nmemb, cmp);
}

void FUNC(crumsort_swap)(void *array, void *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb <= 256)
	{
		FUNC(quadsort_swap)(array, swap, swap_size, nmemb, cmp);
	}
	else
	{
		VAR *pta = (VAR *) array;
		VAR *pts = (VAR *) swap;

		FUNC(crum_analyze)(pta, pts, swap_size, nmemb, cmp);
	}
}
