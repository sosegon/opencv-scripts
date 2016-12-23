/*
/ * main_desktop.cpp
 *
 *  Created on: Dec 17, 2016
 *      Author: sebastian
 */

#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "cartoon.h"
#include "skin.h"

using namespace cv;
using namespace std;

void initWebCam(VideoCapture &videoCapture, int cameraNumber) {
	try {
		videoCapture.open(cameraNumber);
	} catch(cv::Exception &e){

	}
	if (!videoCapture.isOpened()) {
		cerr << "ERROR: Couldn't access the camera or video" << endl;
		exit(1);
	}
	cout << "Loaded camera " << cameraNumber << "." << endl;

}
void captureCamera(int argc, char** argv){
	int cameraNumber = 0;
	if (argc > 1) {
		cameraNumber = atoi(argv[1]);
	}

	VideoCapture camera;
	initWebCam(camera, cameraNumber);

	camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	while (true) {
		Mat cameraFrame;
		camera >> cameraFrame;
		if (cameraFrame.empty()) {
			cerr << "ERROR: Couln't grab camera frame." << endl;
			exit(1);
		}

		Mat displayedFrame = Mat(cameraFrame.size(), CV_8UC3);

		//cartoonifyImage(cameraFrame, displayedFrame);
		hideSkin(cameraFrame, displayedFrame);

		imshow("Original", cameraFrame);
		imshow("Hidden skin", displayedFrame);

		char keypress = cv::waitKey(20);
		if (keypress == 27) {
			break;
		}
	}

}

void captureImage(int argc, char** argv) {
	if (argc != 2) {
		cerr << "ERROR: No image passed" << endl;
		exit(1);
	}

	Mat image = imread(argv[1], CV_LOAD_IMAGE_COLOR);
	if (!image.data) {
		cerr << "ERROR: No image data." << endl;
		exit(1);
	}

	imshow("Original", image);

	Mat displayedFrame;
	displayedFrame = Mat(image.size(), CV_8UC3);

//	cartoonifyImage(image, displayedFrame);
	hideSkin(image, displayedFrame);

	imshow("Hidden skin", displayedFrame);

	while(char(waitKey(1) != 'q')){}

}

int main (int argc, char** argv) {
	captureImage(argc, argv);
//	captureCamera(argc, argv);

	return 0;
}




