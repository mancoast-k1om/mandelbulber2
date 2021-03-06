/*
 * post_effect_hdr_blur.cpp
 *
 *  Created on: 10 mar 2017
 *      Author: krzysztof
 */

#include <QtCore>

#include "post_effect_hdr_blur.h"
#include "cimage.hpp"
#include "progress_text.hpp"
#include "global_data.hpp"

cPostEffectHdrBlur::cPostEffectHdrBlur(cImage *_image) : QObject(), image(_image)
{
	tempImage = new sRGBFloat[image->GetHeight() * image->GetWidth()];
	radius = 0;
	intensity = 0;
}

cPostEffectHdrBlur::~cPostEffectHdrBlur()
{
	delete[] tempImage;
}

void cPostEffectHdrBlur::Render(bool *stopRequest)
{

	memcpy(tempImage, image->GetPostImageFloatPtr(),
		image->GetHeight() * image->GetWidth() * sizeof(sRGBFloat));

	const double blurSize = radius * (image->GetWidth() + image->GetHeight()) * 0.001;
	const double blurSize2 = blurSize * blurSize;
	const int intBlurSize = blurSize + 1;
	const double limiter = intensity;

	QString statusText = QObject::tr("Rendering HDR Blur effect");
	QString progressTxt;

	cProgressText progressText;
	progressText.ResetTimer();
	float percentDone = 0.0;

	QElapsedTimer timerRefreshProgressBar;
	timerRefreshProgressBar.start();

	for (int y = 0; y < image->GetHeight(); y++)
	{
		if(*stopRequest) break;

#pragma omp parallel for
		for (int x = 0; x < image->GetWidth(); x++)
		{
			double weight = 0;
			int yStart = qMax(0, y - intBlurSize);
			int yEnd = qMin(image->GetHeight(), y + intBlurSize);

			sRGBFloat newPixel;

			for (int yy = yStart; yy < yEnd; yy++)
			{
				int xStart = qMax(0, x - intBlurSize);
				int xEnd = qMin(image->GetWidth(), x + intBlurSize);
				for (int xx = xStart; xx < xEnd; xx++)
				{
					double dx = x - xx;
					double dy = y - yy;
					double r2 = dx * dx + dy * dy;
					if (r2 < blurSize2)
					{
						double value = 1.0 / (r2 / (0.2 * blurSize) + limiter);
						// if(dx == 0 && dy == 0) value = 10.0;
						weight += value;
						sRGBFloat oldPixel = tempImage[xx + yy * image->GetWidth()];
						newPixel.R += oldPixel.R * value;
						newPixel.G += oldPixel.G * value;
						newPixel.B += oldPixel.B * value;
					}
				}
			}

			if (weight > 0)
			{
				newPixel.R /= weight;
				newPixel.G /= weight;
				newPixel.B /= weight;
			}
			image->PutPixelPostImage(x, y, newPixel);
		}

		if (timerRefreshProgressBar.elapsed() > 100)
		{
			timerRefreshProgressBar.restart();

			percentDone = float(y) / float(image->GetHeight());
			progressTxt = progressText.getText(percentDone);

			emit updateProgressAndStatus(statusText, progressTxt, percentDone);
			gApplication->processEvents();
		}
	}

	emit updateProgressAndStatus(statusText, progressText.getText(1.0), 1.0);
}

void cPostEffectHdrBlur::SetParameters(double _radius, double _intensity)
{
	radius = _radius;
	intensity = _intensity;
}
