// pirmas_bandymas.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "png.hpp"
#include <random>
#include <chrono>
//#include <ppl.h>

#define W 280
#define H 100
#define L 1500
#define HMult 1200

using namespace std;

double getNextU(double u, double ul, double ur, double uu, double ud, double v, double vl, double vr, double vu, double vd);
double getNextV(double u, double ul, double ur, double uu, double ud, double v, double vl, double vr, double vu, double vd);

double matrixU[W][H]; // [stulpelis][eilute]
double matrixV[W][H]; // [stulpelis][eilute]
double rowU[W];
double rowV[W];

int main()
{
	auto seed = chrono::system_clock::now().time_since_epoch().count();
	normal_distribution<double> distr(0, 0.1);
	default_random_engine re(1);
	for (int i = 0; i < W; i++)
	{
		matrixU[i][0] = distr(re) + 1.0;
		matrixV[i][0] = 0;
	}
	
	for (int i = 1; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			matrixU[j][i] = matrixU[j][i - 1];
			matrixV[j][i] = matrixV[j][i - 1];
		}
		for (int k = 0; k < HMult; k++)
		{
			rowU[0] = getNextU(
				matrixU[0][i],
				matrixU[W - 1][i],
				matrixU[1][i],
				matrixV[0][i],
				matrixV[W - 1][i],
				matrixV[1][i]
			);
			rowV[0] = getNextV(
				matrixU[0][i],
				matrixU[W - 1][i],
				matrixU[1][i],
				matrixV[0][i],
				matrixV[W - 1][i],
				matrixV[1][i]
			);
			
			for (int j = 1; j < W - 1; j++)
			{
				rowU[j] = getNextU(
					matrixU[j][i],
					matrixU[j - 1][i],
					matrixU[j + 1][i],
					matrixV[j][i],
					matrixV[j - 1][i],
					matrixV[j + 1][i]
				);
				rowV[j] = getNextV(
					matrixU[j][i],
					matrixU[j - 1][i],
					matrixU[j + 1][i],
					matrixV[j][i],
					matrixV[j - 1][i],
					matrixV[j + 1][i]
				);
			}
			
			rowU[W - 1] = getNextU(
				matrixU[W - 1][i],
				matrixU[W - 2][i],
				matrixU[0][i],
				matrixV[W - 1][i],
				matrixV[W - 2][i],
				matrixV[0][i]
			);
			rowV[W - 1] = getNextV(
				matrixU[W - 1][i],
				matrixU[W - 2][i],
				matrixU[0][i],
				matrixV[W - 1][i],
				matrixV[W - 2][i],
				matrixV[0][i]
			);
			
			for (int j = 0; j < W; j++)
			{
				matrixU[j][i] = rowU[j];
				matrixV[j][i] = rowV[j];
			}
			
		}
	}
	png::image<png::rgb_pixel> imgU(W, H);
	png::image<png::rgb_pixel> imgV(W, H);

	double maxU = 0;
	double maxV = 0;
	for (int i = 0; i < W; i++)
	{
		for (int j = 0; j < H; j++)
		{
			if (matrixU[i][j] > maxU)
				maxU = matrixU[i][j];
			if (matrixV[i][j] > maxV)
				maxV = matrixV[i][j];
		}
	}
	double multiU = 255 / maxU;
	double multiV = 255 / maxV;
	for (int i = 0; i < W; i++)
	{
		for (int j = 1; j <= H; j++)
		{
			int color = matrixU[i][j - 1] * multiU;
			imgU[H - j][i] = png::rgb_pixel(color, color, color);
			color = matrixV[i][j - 1] * multiV;
			imgV[H - j][i] = png::rgb_pixel(color, color, color);
		}
	}
	imgU.write("U1.png");
	imgV.write("V1.png");
	cout << "U multiplier: " << multiU << endl << "V multiplier: " << multiV;
}

double Du = 0.1;
double chi = 8.3;
double au = 1;
double Bv = 0.73;
double dx2 = 0.075*0.075;
double dy2 = 0.075*0.075;
double dt = 0.00005;

double getNextU(double u, double ul, double ur, double uu, double ud, double v, double vl, double vr, double vu, double vd)
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

double getNextV(double u, double ul, double ur, double uu, double ud, double v, double vl, double vr, double vu, double vd)
{
	return dt * (
		(vr - 2 * v + vl) / dx2
		+ (vu - 2 * v + vd) / dy2
		+ u / (1 + Bv * u)
		- v
		) + v;
}

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
