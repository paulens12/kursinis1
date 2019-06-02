// pirmas_bandymas.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "gif.h"
#include <random>
#include <chrono>
#include <ppl.h>

#define FRAME_DURATION 6
#define W 360
#define H 128
#define L 1500
#define LMult 1200
#define THREADS 6
//THREADS turi dalinti W

using namespace std;

inline double getNextU(double u, double ul, double ur, double uu, double ud, double v, double vl, double vr, double vu, double vd);
inline double getNextV(double u, double v, double vl, double vr, double vu, double vd);

double matrixU[L][H][W]; // [kadras][eilute][stulpelis]
double matrixV[L][H][W]; // [kadras][eilute][stulpelis]

double tempU[LMult + 1][H][W];
double tempV[LMult + 1][H][W];

int main()
{
	auto seed = chrono::system_clock::now().time_since_epoch().count();
	normal_distribution<double> distr(0, 0.1);
	default_random_engine re(1);
	for (int i = 0; i < H; i++)
	{
		for(int j = 0; j < W; j++)
		{
			matrixU[0][i][j] = distr(re) + 1.0;
			matrixV[0][i][j] = 0;
		}
	}

	if (W % THREADS != 0) cout << "DEMESIO!!! THREADS nedalija W!";

	atomic<int> done = 0;
	bool cont[THREADS];
	std::fill(cont, cont + THREADS, true);
	
	memcpy(tempU[0], matrixU[0], W * H * sizeof(double));
	memcpy(tempV[0], matrixV[0], W * H * sizeof(double));

	int width = W / THREADS;

	concurrency::parallel_for(0, THREADS, [&](int thn)
	{
		int rangeBegin = thn * width;
		for (int i = 0; i < L - 1; i++)
		{
			for (int ii = 1; ii <= LMult; ii++)
			{
				while (!cont[thn]) {}
				cont[thn] = false;
				if (ii == 1)
				{
					for (int k = 0; k < H; k++)
					{
						memcpy(tempU[0][k] + rangeBegin, matrixU[i][k] + rangeBegin, width * sizeof(double));
						memcpy(tempV[0][k] + rangeBegin, matrixV[i][k] + rangeBegin, width * sizeof(double));
					}
				}
				for (int j = rangeBegin; j < rangeBegin + width; j++)
				{
					for (int k = 1; k < H - 1; k++)
					{
						int l, r, u, d;

						if (j == 0)
							l = W - 1;
						else
							l = j - 1;
						if (j == W - 1)
							r = 0;
						else
							r = j + 1;
						d = k - 1;
						u = k + 1;

						tempU[ii][k][j] = getNextU(tempU[ii - 1][k][j], tempU[ii - 1][k][l], tempU[ii - 1][k][r], tempU[ii - 1][u][j], tempU[ii - 1][d][j], tempV[ii - 1][k][j], tempV[ii - 1][k][l], tempV[ii - 1][k][r], tempV[ii - 1][u][j], tempV[ii - 1][d][j]);
						tempV[ii][k][j] = getNextV(tempU[ii - 1][k][j], tempV[ii - 1][k][j], tempV[ii - 1][k][l], tempV[ii - 1][k][r], tempV[ii - 1][u][j], tempV[ii - 1][d][j]);
					}
					tempU[ii][0][j] = (4 * tempU[ii][1][j] - tempU[ii][2][j]) / 3;
					tempV[ii][0][j] = (4 * tempV[ii][1][j] - tempV[ii][2][j]) / 3;
					tempU[ii][H - 1][j] = (4 * tempU[ii][H - 2][j] - tempU[ii][H - 3][j]) / 3;
					tempV[ii][H - 1][j] = (4 * tempV[ii][H - 2][j] - tempV[ii][H - 3][j]) / 3;
				}
				done++;

				if (thn == 0)
				{
					while (done != THREADS) {}
					done = 0;
					if (ii == LMult)
					{
						memcpy(matrixU[i + 1], tempU[ii], W * H * sizeof(double));
						memcpy(matrixV[i + 1], tempV[ii], W * H * sizeof(double));
						if (i % 10 == 0) cout << i << endl;
					}
					for (int k = 0; k < THREADS; k++)
						cont[k] = true;
				}
			}
		}
	});
	

	double maxU = 0;
	double maxV = 0;
	for (int k = 0; k < L; k++)
	{
		for (int i = 0; i < H; i++)
		{
			for (int j = 0; j < W; j++)
			{
				if (matrixU[k][i][j] > maxU)
					maxU = matrixU[k][i][j];
				if (matrixV[k][i][j] > maxV)
					maxV = matrixV[k][i][j];
			}
		}
	}
		
	double multiU = 255 / maxU;
	double multiV = 255 / maxV;

	uint8_t frameU[W * H * 4];
	uint8_t frameV[W * H * 4];

	GifWriter gu;
	GifWriter gv;
	GifBegin(&gu, "U2.gif", W, H, FRAME_DURATION);
	GifBegin(&gv, "V2.gif", W, H, FRAME_DURATION);

	for (int i = 0; i < H * W; i++)
	{
		frameU[i * 4 + 3] = 0;
		frameV[i * 4 + 3] = 0;
	}
	for (int k = 0; k < L; k++)
	{
		for (int i = 0; i < H; i++)
		{
			for (int j = 0; j < W; j++)
			{
				frameU[4 * (W * i + j)] = multiU * matrixU[k][i][j];
				frameU[4 * (W * i + j)+1] = multiU * matrixU[k][i][j];
				frameU[4 * (W * i + j)+2] = multiU * matrixU[k][i][j];
				frameV[4 * (W * i + j)] = multiV * matrixV[k][i][j];
				frameV[4 * (W * i + j) + 1] = multiV * matrixV[k][i][j];
				frameV[4 * (W * i + j) + 2] = multiV * matrixV[k][i][j];
			}
		}
		GifWriteFrame(&gu, frameU, W, H, FRAME_DURATION);
		GifWriteFrame(&gv, frameV, W, H, FRAME_DURATION);
	}
	GifEnd(&gu);
	GifEnd(&gv);
	cout << "U multiplier: " << multiU << endl << "V multiplier: " << multiV << endl << "U max: " << maxU << endl << "V max: " << maxV;
}

double Du = 0.1;
double chi = 8.3;
double au = 1;
double Bv = 0.73;
double dx2 = 0.075*0.075;
double dy2 = 0.075*0.075;
double dt = 0.00005;

inline double getNextU(double u, double ul, double ur, double uu, double ud, double v, double vl, double vr, double vu, double vd)
{
	double ul2 = (u + ul) / 2;
	double ur2 = (u + ur) / 2;
	double uu2 = (u + uu) / 2;
	double ud2 = (u + ud) / 2;

	return dt * (
			Du * (
				(ur - 2 * u + ul) / dx2
				+ (uu - 2 * u + ud) / dy2
			)
			- chi * (
				(ur2 * (vr - v) - ul2 * (v - vl)) / dx2
				+ (uu2 * (vu - v) - ud2 * (v - vd)) / dy2
			)
			+ au * u * (1 - u)
		) + u;
}

inline double getNextV(double u, double v, double vl, double vr, double vu, double vd)
{
	return dt * (
		(vr - 2 * v + vl) / dx2
		+ (vu - 2 * v + vd) / dy2
		+ u / (1 + Bv * u)
		- v
		) + v;
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