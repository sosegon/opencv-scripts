#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char *argv[])
{
    Mat img = imread("lena.jpg", CV_LOAD_IMAGE_GRAYSCALE);
    if(img.empty())
       return -1;

    // Noise
    Mat mean = Mat::zeros(1, 1, CV_64FC1);
    Mat sigma = Mat::ones(1, 1, CV_64FC1);
    Mat noise = Mat(img.size(), CV_64FC1);
    Mat noise2 = Mat(img.size(), CV_8UC1);

    randn(noise, mean, sigma);
    noise = noise * 50;

    noise.convertTo(noise2, CV_8UC1);

    Mat result = noise2 + img;

    // Smooth
    Size kernel = Size(21, 21);
    GaussianBlur(result, result, kernel, 1, 60);

    namedWindow( "lena", CV_WINDOW_AUTOSIZE );
    imshow("lena", result);
    waitKey(0);
    return 0;
}



