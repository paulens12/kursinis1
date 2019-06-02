// antras_bandymas.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "png.hpp"
#include <random>
#include <chrono>
#include <ppl.h>

#define W 360
#define H 1500
#define LMult 1200
#define THREADS 6
//THREADS turi dalinti W

using namespace std;

double getNextU(double prevU, double prevUL, double prevUR, double prevV, double prevVL, double prevVR);
double getNextV(double prevU, double prevUL, double prevUR, double prevV, double prevVL, double prevVR);

double matrixU[H][W]; // [eilute][stulpelis]
double matrixV[H][W]; // [eilute][stulpelis]

double tempU[LMult+1][W];
double tempV[LMult+1][W];

int main()
{
	auto seed = chrono::system_clock::now().time_since_epoch().count();
	normal_distribution<double> distr(0, 0.1);
	default_random_engine re(1);
	for (int i = 0; i < W; i++)
	{
		matrixU[0][i] = distr(re) + 1.0;
		matrixV[0][i] = 0;
	}

	if (W % THREADS != 0) cout << "DEMESIO!!! THREADS nedalija W!";

	atomic<int> done = 0;
	bool cont[THREADS];
	std::fill(cont, cont + THREADS, true);

	memcpy(tempU[0], matrixU[0], W * sizeof(double));
	memcpy(tempV[0], matrixV[0], W * sizeof(double));

	int width = W / THREADS;
	concurrency::parallel_for(0, THREADS, [&](int thn)
	{
		int rangeBegin = thn * width;
		for (int i = 0; i < H - 1; i++)
		{
			for (int ii = 1; ii <= LMult; ii++)
			{
				while (!cont[thn]) { }
				cont[thn] = false;
				if (ii == 1)
				{
					memcpy(tempU[0] + rangeBegin, matrixU[i] + rangeBegin, width * sizeof(double));
					memcpy(tempV[0] + rangeBegin, matrixV[i] + rangeBegin, width * sizeof(double));
				}
				for (int j = rangeBegin; j < rangeBegin + width; j++)
				{
					int prev;

					if (j == 0)
						prev = W - 1;
					else
						prev = j - 1;

					int next;
					if (j == W - 1)
						next = 0;
					else
						next = j + 1;

					tempU[ii][j] = getNextU(tempU[ii - 1][j], tempU[ii - 1][prev], tempU[ii - 1][next], tempV[ii - 1][j], tempV[ii - 1][prev], tempV[ii - 1][next]);
					tempV[ii][j] = getNextV(tempU[ii - 1][j], tempU[ii - 1][prev], tempU[ii - 1][next], tempV[ii - 1][j], tempV[ii - 1][prev], tempV[ii - 1][next]);
				}
				done++;

				if (thn == 0)
				{
					while (done != THREADS) {}
					done = 0;
					if (ii == LMult)
					{
						memcpy(matrixU[i + 1], tempU[ii], W * sizeof(double));
						memcpy(matrixV[i + 1], tempV[ii], W * sizeof(double));
					}
					for (int k = 0; k < THREADS; k++)
					{
						cont[k] = true;
					}
				}
			}
		}
	});

	png::image<png::rgb_pixel> imgU(W, H);
	png::image<png::rgb_pixel> imgV(W, H);

	double maxU = 0;
	double maxV = 0;
	for (int i = 0; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			if (matrixU[i][j] > maxU)
				maxU = matrixU[i][j];
			if (matrixV[i][j] > maxV)
				maxV = matrixV[i][j];
		}
	}
	double multiU = 255 / maxU;
	double multiV = 255 / maxV;

	for (int i = 1; i <= H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			int color = matrixU[i - 1][j] * multiU;
			imgU[H - i][j] = png::rgb_pixel(color, color, color);
			color = matrixV[i - 1][j] * multiV;
			imgV[H - i][j] = png::rgb_pixel(color, color, color);
		}
	}
	imgU.write("U2.png");
	imgV.write("V2.png");
	cout << "U multiplier: " << multiU << endl << "V multiplier: " << multiV << endl << "U max: " << maxU << endl << "V max: " << maxV;
}

double Du = 0.1;
double chi = 8.3;
double au = 1;
double Bv = 0.73;
double dx2 = 0.075*0.075;
double dt = 0.00005;

double getNextU(double prevU, double prevUL, double prevUR, double prevV, double prevVL, double prevVR)
{
	double prevUL2 = (prevU + prevUL) / 2;
	double prevUR2 = (prevU + prevUR) / 2;
	return	(
		(prevUR - 2 * prevU + prevUL) * Du / dx2
		- (
			prevUR2 * (prevVR - prevV)
			- prevUL2 * (prevV - prevVL)
			) * chi / dx2
		+ au * prevU * (1 - prevU)
		) * dt + prevU;
}

double getNextV(double prevU, double prevUL, double prevUR, double prevV, double prevVL, double prevVR)
{
	return	(
		(prevVR - 2 * prevV + prevVL) / dx2
		+ prevU / (1 + Bv * prevU)
		- prevV
		) * dt + prevV;
}