/*
 * cartoon.cpp
 *
 *  Created on: Dec 17, 2016
 *      Author: sebastian
 */
#include "cartoon.h"

void cartoonifyImage(Mat srcColor, Mat dstColor) {
	// Use a grayscale image to apply Laplacian edge-detection filter
	Mat gray;
	cvtColor(srcColor, gray, CV_BGR2GRAY);

	// Median blur filter used to remove noise. Faster than
	// bilateral filter. Keep edges sharp
	const int MEDIAN_BLUR_FILTER_SIZE = 7;
	medianBlur(gray, gray, MEDIAN_BLUR_FILTER_SIZE);

	// Apply Laplacian filter to detect edges
	Mat edges;
	const int LAPLACIAN_FILTER_SIZE = 5;
	Laplacian(gray, edges, CV_8U, LAPLACIAN_FILTER_SIZE);

	// Laplacian filter creates edges with different brightness
	// Binary threshold outputs black and white image
	Mat mask;
	const int EDGES_THRESHOLD = 80;
	threshold(edges, mask, EDGES_THRESHOLD, 255, THRESH_BINARY_INV);

	// Reduce size of image to apply bilateral filter
	Size size = srcColor.size();
	Size smallSize;
	smallSize.width = size.width / 2;
	smallSize.height = size.height / 2;
	Mat smallImage = Mat(smallSize, CV_8UC3);
	resize(srcColor, smallImage, smallSize, 0, 0, INTER_LINEAR);

	// Apply bilateral filter to smooth flat areas
	Mat tmp = Mat(smallSize, CV_8UC3); // result of the bilateral filter
	int repetitions = 7; // repetitions to apply the filter
	for (int i = 0; i < repetitions; i++) {
		int ksize = 9; // filter size
		double sigmaColor = 9; // filter color strength
		double sigmaSpace = 7; // spatial strength, affects speed.
		bilateralFilter(smallImage, tmp, ksize, sigmaColor, sigmaSpace);
		bilateralFilter(tmp, smallImage, ksize, sigmaColor, sigmaSpace);
	}

	// Rescale image to original size
	Mat bigImage;
	resize(smallImage, bigImage, size, 0, 0, INTER_LINEAR);
	dstColor.setTo(0);
	bigImage.copyTo(dstColor, mask);


	//	Mat o = Mat::ones(srcColor.size(), CV_8UC3);
	//	o *= 255;
	//	o.copyTo(dstColor, mask);
//		srcColor.copyTo(dstColor, mask);


}



