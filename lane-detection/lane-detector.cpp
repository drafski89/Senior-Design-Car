#include <cstdio>
#include <cstdlib>
#include <string>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Specify an image file to detect lanes in\n");
        return EXIT_SUCCESS;
    }

    cv::Mat img_color = cv::imread(string(argv[1]), cv::IMREAD_COLOR);
    if (img_color.empty())
    {
        printf("Failed to read image file \'%s\'. Exiting\n", argv[1]);
        return EXIT_FAILURE;
    }

    cv::Mat img_gray;
    cv::cvtColor(img_color, img_gray, cv::COLOR_BGR2GRAY);

    cv::Mat edge_img;
    cv::Canny(img_gray, edge_img, 40.0, 80.0);

    cv::imwrite(string("edge_img.jpg"), edge_img);

    return EXIT_SUCCESS;
}
