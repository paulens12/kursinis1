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
#define HMult 12000
#define THREADS 4
//THREADS turi dalinti W

using namespace std;

double getNextU(double prevU, double prevUL, double prevUR, double prevV, double prevVL, double prevVR);
double getNextV(double prevU, double prevUL, double prevUR, double prevV, double prevVL, double prevVR);

double matrixU[H][W]; // [eilute][stulpelis]
double matrixV[H][W]; // [eilute][stulpelis]

double tempU[2][W];
double tempV[2][W];

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
	atomic<int> done = 0;

	bool cont[THREADS];
	mutex locks[THREADS];
	condition_variable cvs[THREADS];
	std::fill(cont, cont + THREADS, true);

	memcpy(tempU[0], matrixU[0], W * sizeof(double));
	memcpy(tempV[0], matrixV[0], W * sizeof(double));

	int width = W / THREADS;
	concurrency::parallel_for(0, THREADS, [&](int thn)
	{
		int rangeBegin = thn * width;
		for (int i = 0; i < H - 1; i++)
		{
			//memcpy(tempU[0] + rangeBegin, matrixU[i] + rangeBegin, width * sizeof(double));
			//memcpy(tempV[0] + rangeBegin, matrixV[i] + rangeBegin, width * sizeof(double));
			for (int ii = 0; ii < HMult; ii++)
			{
				while (!cont[thn]) { }
				cont[thn] = false;
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

					tempU[1][j] = getNextU(tempU[0][j], tempU[0][prev], tempU[0][next], tempV[0][j], tempV[0][prev], tempV[0][next]);
					tempV[1][j] = getNextV(tempU[0][j], tempU[0][prev], tempU[0][next], tempV[0][j], tempV[0][prev], tempV[0][next]);
				}
				done++;

				if (thn == 0)
				{
					while (done != THREADS) {}
					memcpy(tempU[0], tempU[1], W * sizeof(double));
					memcpy(tempV[0], tempV[1], W * sizeof(double));
					done = 0;
					for (int k = 0; k < THREADS; k++)
					{
						cont[k] = true;
						//unique_lock<mutex> l(locks[k]);
						//cont[k] = true;
						//cvs[k].notify_one();
					}
					if (ii == HMult - 1)
					{
						memcpy(matrixU[i + 1], tempU[0], W * sizeof(double));
						memcpy(matrixV[i + 1], tempV[0], W * sizeof(double));
					}
				}
				//unique_lock<mutex> l(locks[thn]);
					//while (!cont[thn])
					//	cvs[thn].wait(l);
				//if(!cont[thn])
				//	cvs[thn].wait(l, [&] {return cont[thn]; });
				//cont[thn] = false;
			}

			//if(i%100 == 0)
			//	cout << i << endl;

			//memcpy(matrixU[i + 1] + rangeBegin, tempU[0] + rangeBegin, width * sizeof(double));
			//memcpy(matrixV[i + 1] + rangeBegin, tempV[0] + rangeBegin, width * sizeof(double));
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

	for (int i = 1; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			int color = matrixU[i - 1][j] * multiU;
			imgU[H - i][j] = png::rgb_pixel(color, color, color);
			color = matrixV[i - 1][j] * multiV;
			imgV[H - i][j] = png::rgb_pixel(color, color, color);
		}
	}
	imgU.write("U.png");
	imgV.write("V.png");
	cout << "U multiplier: " << multiU << endl << "V multiplier: " << multiV;
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

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
