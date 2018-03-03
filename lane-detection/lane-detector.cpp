#include <cstdio>
#include <cstdlib>
#include <cmath>
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

    int hres = 1920;
    int vres = (int)((double)img_color.rows * ((double)hres / img_color.cols));
    cv::resize(img_color, img_color, cv::Size(hres, vres), cv::INTER_AREA);

    // remove noise
    cv::medianBlur(img_color, img_color, 21);
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

    vector<cv::Vec2d> lines;
    // 25.0 pix radius granularity, 1 deg angular granularity, 200 votes min for a line
    // 200 pixels min for a segment, up to 300 pixels between disconnected colinear segments
    cv::HoughLines(edge_img, lines, 10.0, 4.0 * CV_PI / 180.0, 400);

    printf("Found %lu lines in the image\n", lines.size());

    for (auto& line : lines)
    {
        printf("  Radius: %f    Theta: %f\n", line[0], line[1] / CV_PI * 180.0);

        if ((line[1] > 0.2) && (line[1] < CV_PI - 0.2) &&
            ((line[1] < 7.5 * CV_PI / 18.0) || (line[1] > 11.5 * CV_PI / 18.0)))
        {
            double slope = -1.0 / tan(line[1]);
            double y_init = line[0] * sin(line[1]);
            double x_init = line[0] * cos(line[1]);
            double x1, y1, x2, y2 = 0.0;

            x1 = 0.0;
            y1 = -slope * x_init + y_init;

            if (slope < 0.0)
            {
                x2 = -y_init / slope + x_init;
                y2 = 0.0;
            }
            else
            {
                x2 = (double)edge_img.cols;
                y2 = slope * x2 - slope * x_init + y_init;
            }

            cv::line(edge_img,
                     cv::Point2i((int)x1, (int)y1),
                     cv::Point2i((int)x2, (int)y2),
                     cv::Scalar(255.0),
                     10);

            printf("(%f, %f), (%f, %f)\n", x1, y1, x2, y2);
        }
    }

    // write out images for analysis
    cv::imwrite(string("blur_img.jpg"), img_color);
    cv::imwrite(string("edge_img.jpg"), edge_img);

    return EXIT_SUCCESS;
}
