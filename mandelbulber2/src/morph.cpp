/**
 * Mandelbulber v2, a 3D fractal generator
 *
 * Class to interpolate parameters for animation
 *
 * Copyright (C) 2014 Krzysztof Marczak
 *
 * This file is part of Mandelbulber.
 *
 * Mandelbulber is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Mandelbulber is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details. You should have received a copy of the GNU
 * General Public License along with Mandelbulber. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Sebastian Jennen (jenzebas@gmail.com), Krzysztof Marczak (buddhi1980@gmail.com)
 */

#include <math.h>
#include <stdio.h>
#include "morph.hpp"
#include "algebra.hpp"
#include "common_math.h"
#include <gsl/gsl_interp.h>
#include <gsl/gsl_spline.h>

cMorph::cMorph()
{
	listSize = 6;
	interpolationAccelerator = gsl_interp_accel_alloc ();
	splineAkimaPeriodic = gsl_spline_alloc (gsl_interp_akima_periodic, listSize);
}

cMorph::~cMorph()
{
	gsl_spline_free(splineAkimaPeriodic);
	gsl_interp_accel_free(interpolationAccelerator);
}

void cMorph::AddData(const int keyFrame, const cOneParameter val)
{
	int key = findInMorph(keyFrame);
	if(key != -1) return;
	sMorphParameter mVal(keyFrame, val);
	dataSets.append(mVal);

	if(dataSets.size() > listSize){
		dataSets.removeFirst();
	}
}

int cMorph::findInMorph(const int keyframe)
{
	for(int i = 0; i < dataSets.size(); i++)
	{
		if(dataSets[i].keyframe == keyframe)
		{
			return i;
		}
	}
	return -1;
}

cOneParameter cMorph::Interpolate(const int keyframe, float factor)
{
	int key = findInMorph(keyframe);
	if(key == -1) return dataSets[0].parameter;
	if(dataSets[key].parameter.GetValueType() == typeString || dataSets[key].parameter.GetValueType() == typeBool)
	{
		return None(key);
	}

	switch(dataSets[0].parameter.GetMorphType())
	{
	case morphNone:
		return None(key);
	case morphLinear:
		return Linear(key, factor);
	case morphCatMullRom:
		return CatmullRom(key, factor);
	case morphCatMullRomAngle:
		return CatmullRomAngular(key, factor);
	default:
		return None(key);
	}
}

cOneParameter cMorph::None(const int key)
{
	return dataSets[key].parameter;
}

cOneParameter cMorph::Linear(const int key, const double factor)
{
	int k1, k2;
	if(key == dataSets.size() - 1) return dataSets[key].parameter;

	cOneParameter interpolated = dataSets[key].parameter;
	cMultiVal val;

	k1 = key;
	k2 = key + 1;

	switch(dataSets[key].parameter.GetValueType())
	{
	case typeNull:
	case typeString:
	case typeBool:
		return None(key);

	case typeDouble:
	case typeInt:
	{
		double v1, v2;
		dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
		dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
		val.Store(LinearInterpolate(factor, v1, v2));
		break;
	}
	case typeRgb:
	{
		sRGB v1, v2;
		dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
		dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
		val.Store(
					sRGB(
						LinearInterpolate(factor, v1.R, v2.R),
						LinearInterpolate(factor, v1.G, v2.G),
						LinearInterpolate(factor, v1.B, v2.B)
						)
					);
		break;
	}
	case typeVector3:
	{
		CVector3 v1, v2;
		dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
		dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
		val.Store(
					CVector3(
						LinearInterpolate(factor, v1.x, v2.x),
						LinearInterpolate(factor, v1.y, v2.y),
						LinearInterpolate(factor, v1.z, v2.z)
						)
					);
		break;
	}
	case typeColorPalette:
	{
		cColorPalette v1, v2;
		cColorPalette out;
		dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
		dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
		for(int i = 0; i < v1.GetSize(); i++)
		{
			out.AppendColor(
						sRGB(
							LinearInterpolate(factor, v1.GetColor(i).R, v2.GetColor(i).R),
							LinearInterpolate(factor, v1.GetColor(i).G, v2.GetColor(i).G),
							LinearInterpolate(factor, v1.GetColor(i).B, v2.GetColor(i).B)
							)
						);
		}
		val.Store(out);
		break;
	}
		//  TODO typeColorPalette
	}

	interpolated.SetMultival(val, valueActual);
	return interpolated;
}

cOneParameter cMorph::CatmullRom(int const key, double const factor)
{
	int k1, k2, k3, k4;

	if (key >= 1) k1 = key - 1;
	else k1 = key;

	if (key < dataSets.size()) k2 = key;
	else k2 = dataSets.size() - 1;

	if (key < dataSets.size() - 1) k3 = key + 1;
	else k3 = dataSets.size() - 1;

	if (key < dataSets.size() - 2) k4 = key + 2;
	else k4 = dataSets.size() - 1;

	cOneParameter interpolated = dataSets[key].parameter;
	cMultiVal val;

	switch(dataSets[key].parameter.GetValueType())
	{
		case typeNull:
		case typeString:
		case typeBool:
			return None(key);

		case typeDouble:
		case typeInt:
		{
			double v1, v2, v3, v4;
			dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
			dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
			dataSets[k3].parameter.GetMultival(valueActual).Get(v3);
			dataSets[k4].parameter.GetMultival(valueActual).Get(v4);
			val.Store(CatmullRomInterpolate(factor, v1, v2, v3, v4));
			break;
		}
		case typeRgb:
		{
			sRGB v1, v2, v3, v4;
			dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
			dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
			dataSets[k3].parameter.GetMultival(valueActual).Get(v3);
			dataSets[k4].parameter.GetMultival(valueActual).Get(v4);
			val.Store(
						sRGB(
							CatmullRomInterpolate(factor, v1.R, v2.R, v3.R, v4.R),
							CatmullRomInterpolate(factor, v1.G, v2.G, v3.G, v4.G),
							CatmullRomInterpolate(factor, v1.B, v2.B, v3.B, v4.B)
							)
						);
			break;
		}
		case typeVector3:
		{
			CVector3 v1, v2, v3, v4;
			dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
			dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
			dataSets[k3].parameter.GetMultival(valueActual).Get(v3);
			dataSets[k4].parameter.GetMultival(valueActual).Get(v4);
			val.Store(
						CVector3(
							CatmullRomInterpolate(factor, v1.x, v2.x, v3.x, v4.x),
							CatmullRomInterpolate(factor, v1.y, v2.y, v3.y, v4.y),
							CatmullRomInterpolate(factor, v1.z, v2.z, v3.z, v4.z)
							)
						);
			break;
		}
		case typeColorPalette:
			//  TODO typeColorPalette
			return None(key);
	}

	interpolated.SetMultival(val, valueActual);
	return interpolated;
}

cOneParameter cMorph::Akima(int const key, double const factor)
{
	int k1, k2, k3, k4, k5, k6;

	if (key >= 2) k1 = key - 2;
	else if (key >= 1) k1 = key - 1;
	else k1 = key;

	if (key >= 1) k2 = key - 1;
	else k2 = key;

	if (key < dataSets.size()) k3 = key;
	else k3 = dataSets.size() - 1;

	if (key < dataSets.size() - 1) k4 = key + 1;
	else k4 = dataSets.size() - 1;

	if (key < dataSets.size() - 2) k5 = key + 2;
	else k5 = dataSets.size() - 1;

	if (key < dataSets.size() - 2) k6 = key + 3;
	else k6 = dataSets.size() - 1;

	cOneParameter interpolated = dataSets[key].parameter;
	cMultiVal val;

	switch(dataSets[key].parameter.GetValueType())
	{
		case typeNull:
		case typeString:
		case typeBool:
			return None(key);

		case typeDouble:
		case typeInt:
		{
			double v1, v2, v3, v4, v5, v6;
			dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
			dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
			dataSets[k3].parameter.GetMultival(valueActual).Get(v3);
			dataSets[k4].parameter.GetMultival(valueActual).Get(v4);
			dataSets[k5].parameter.GetMultival(valueActual).Get(v5);
			dataSets[k6].parameter.GetMultival(valueActual).Get(v6);
			val.Store(AkimaInterpolate(factor, v1, v2, v3, v4, v5, v6));
			break;
		}
		case typeRgb:
		{
			sRGB v1, v2, v3, v4, v5, v6;
			dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
			dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
			dataSets[k3].parameter.GetMultival(valueActual).Get(v3);
			dataSets[k4].parameter.GetMultival(valueActual).Get(v4);
			dataSets[k5].parameter.GetMultival(valueActual).Get(v5);
			dataSets[k6].parameter.GetMultival(valueActual).Get(v6);
			val.Store(
						sRGB(
							AkimaInterpolate(factor, v1.R, v2.R, v3.R, v4.R, v5.R, v6.R),
							AkimaInterpolate(factor, v1.G, v2.G, v3.G, v4.G, v5.G, v6.G),
							AkimaInterpolate(factor, v1.B, v2.B, v3.B, v4.B, v5.B, v6.B)
							)
						);
			break;
		}
		case typeVector3:
		{
			CVector3 v1, v2, v3, v4, v5, v6;
			dataSets[k1].parameter.GetMultival(valueActual).Get(v1);
			dataSets[k2].parameter.GetMultival(valueActual).Get(v2);
			dataSets[k3].parameter.GetMultival(valueActual).Get(v3);
			dataSets[k4].parameter.GetMultival(valueActual).Get(v4);
			dataSets[k5].parameter.GetMultival(valueActual).Get(v5);
			dataSets[k6].parameter.GetMultival(valueActual).Get(v6);
			val.Store(
						CVector3(
							AkimaInterpolate(factor, v1.x, v2.x, v3.x, v4.x, v5.x, v6.x),
							AkimaInterpolate(factor, v1.y, v2.y, v3.y, v4.y, v5.y, v6.y),
							AkimaInterpolate(factor, v1.z, v2.z, v3.z, v4.z, v5.z, v6.z)
							)
						);
			break;
		}
		case typeColorPalette:
			//  TODO typeColorPalette
			return None(key);
	}

	interpolated.SetMultival(val, valueActual);
	return interpolated;
}

cOneParameter cMorph::CatmullRomAngular(const int frame, const double factor)
{
	// different?
	return CatmullRom(frame, factor);
}

double cMorph::LinearInterpolate(const double factor, const double v1, const double v2)
{
	return v1 + ((v2 - v1) * factor);
}

double cMorph::AkimaInterpolate(const double factor, double v1, double v2, double v3, double v4, double v5, double v6)
{
	// more info: http://www.alglib.net/interpolation/spline3.php
	double x[] = {-2, -1, 0, 1, 2, 3};
	double y[] = {v1, v2, v3, v4, v5, v6};

	gsl_spline_init(splineAkimaPeriodic, x, y, listSize);
	return gsl_spline_eval(splineAkimaPeriodic, factor, interpolationAccelerator);
}

double cMorph::CatmullRomInterpolate(const double factor, double v1, double v2, double v3, double v4)
{
	double factor2 = factor * factor;
	double factor3 = factor2 * factor;

	bool logaritmic = false;
	bool negative = false;
	if ((v1 > 0 && v2 > 0 && v3 > 0 && v4 > 0) || (v1 < 0 && v2 < 0 && v3 < 0 && v4 < 0))
	{
		if(v1 < 0) negative = true;
		double average = (v1 + v2 + v3 + v4) / 4.0;
		if (average > 0)
		{
			double deviation = (fabs(v2 - v1) + fabs(v3 - v2) + fabs(v4 - v3)) / average;
			if(deviation > 0.1)
			{
				v1 = log(fabs(v1));
				v2 = log(fabs(v2));
				v3 = log(fabs(v3));
				v4 = log(fabs(v4));
				logaritmic = true;
			}
		}
	}
	double value = 0.5 * ((2 * v2) + (-v1 + v3) * factor + (2 * v1 - 5 * v2 + 4 * v3 - v4) * factor2 + (-v1 + 3 * v2 - 3 * v3 + v4) * factor3);
	if(logaritmic)
	{
		if(negative) value = -exp(value);
		else value = exp(value);
	}
	if(value > 1e20) value = 1e20;
	if(value < -1e20) value = 1e20;
	if(fabs(value) < 1e-20) value = 0.0;

	return value;
}