#include <cstdio>
#include <cstdlib>
#include <vector>
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

    // remove noise
    cv::medianBlur(img_color, img_color, 25);
    //cv::GaussianBlur(img_color, img_color, cv::Size(25, 25), 15, 1);

    // convert image to grayscale
    cv::Mat img_gray;
    cv::cvtColor(img_color, img_gray, cv::COLOR_BGR2GRAY);

    // perform canny edge detection
    cv::Mat edge_img;
    cv::Canny(img_gray, edge_img, 40.0, 80.0);

    // blot out top half of image
    cv::rectangle(edge_img,
                  cv::Point2i(0, 0),
                  cv::Point2i(img_gray.cols - 1, img_gray.rows / 2),
                  cv::Scalar(0.0),
                  cv::FILLED);

    vector<cv::Vec4i> lines;
    // 25.0 pix radius granularity, 1 deg angular granularity, 200 votes min for a line
    // 200 pixels min for a segment, up to 300 pixels between disconnected colinear segments
    cv::HoughLinesP(edge_img, lines, 25.0, CV_PI / 180.0, 200, 200, 300);

    printf("Found %lu lines in the image\n", lines.size());

    for (auto& line : lines)
    {
        printf("  Start: (%d, %d)    End: (%d, %d)\n", line[0], line[1], line[2], line[3]);
        cv::line(edge_img,
                 cv::Point2i(line[0], line[1]),
                 cv::Point2i(line[2], line[3]),
                 cv::Scalar(255.0),
                 10);
    }

    // write out images for analysis
    cv::imwrite(string("blur_img.jpg"), img_color);
    cv::imwrite(string("edge_img.jpg"), edge_img);

    return EXIT_SUCCESS;
}
