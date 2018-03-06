#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <sys/time.h>
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

    struct timeval now;
    gettimeofday(&now, NULL);
    unsigned long start_time = now.tv_sec * 1000 + now.tv_usec / 1000;

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
                  cv::Point2i(0, img_gray.rows / 2),
                  cv::Point2i(img_gray.cols - 1, img_gray.rows),
                  cv::Scalar(0.0),
                  cv::FILLED);

    vector<cv::Vec2d> lines;
    // 25.0 pix radius granularity, 1 deg angular granularity, 200 votes min for a line
    // 200 pixels min for a segment, up to 300 pixels between disconnected colinear segments
    cv::HoughLines(edge_img, lines, 10.0, 4.0 * CV_PI / 180.0, 400);

    printf("Found %lu lines in the image\n", lines.size());
    vector<cv::Vec2d> lane_lines_left;
    vector<cv::Vec2d> lane_lines_right;

    for (auto& line : lines)
    {
        printf("  Radius: %f    Theta: %f\n", line[0], line[1] / CV_PI * 180.0);

        if ((line[1] > 0.2) && (line[1] < CV_PI - 0.2) &&
            ((line[1] < 8.0 * CV_PI / 18.0) || (line[1] > 10.0 * CV_PI / 18.0)))
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
                lane_lines_left.push_back(cv::Vec2d(slope, -slope * x_init + y_init));
            }
            else
            {
                x2 = (double)edge_img.cols;
                y2 = slope * x2 - slope * x_init + y_init;
                lane_lines_right.push_back(cv::Vec2d(slope, -slope * x_init + y_init));
            }

            cv::line(edge_img,
                     cv::Point2i((int)x1, (int)y1),
                     cv::Point2i((int)x2, (int)y2),
                     cv::Scalar(255.0),
                     10);

            printf("(%f, %f), (%f, %f)\n", x1, y1, x2, y2);
        }
    }


    if (!lane_lines_left.empty() && !lane_lines_right.empty())
    {
        cv::Vec2d lane_left(lane_lines_left[0][0], lane_lines_left[0][1]);
        cv::Vec2d lane_right(lane_lines_right[0][0], lane_lines_right[0][1]);

        for (int index = 1; index < lane_lines_left.size(); index++)
        {
            lane_left[0] = (lane_left[0] + lane_lines_left[index][0]) / 2.0;
            lane_left[1] = (lane_left[1] + lane_lines_left[index][1]) / 2.0;
        }

        for (int index = 1; index < lane_lines_right.size(); index++)
        {
            lane_right[0] = (lane_right[0] + lane_lines_right[index][0]) / 2.0;
            lane_right[1] = (lane_right[1] + lane_lines_right[index][1]) / 2.0;
        }

        int lane_start_y = edge_img.rows;
        int lane_left_start_x = ((double)lane_start_y - lane_left[1]) / lane_left[0];
        int lane_right_start_x = ((double)lane_start_y - lane_right[1]) / lane_right[0];
        int lane_center = lane_left_start_x + ((double)lane_right_start_x - lane_left_start_x) / 2.0;
        int xint = (lane_right[1] - lane_left[1]) / (lane_left[0] - lane_right[0]);
        int yint = lane_right[0] * xint + lane_right[0];

        cv::line(edge_img,
                 cv::Point2i(xint, yint),
                 cv::Point2i(lane_center, lane_start_y),
                 cv::Scalar(255.0),
                 5);

        printf("\nDistance from Center: %d px\n"
               "Right / Left X: %d, %d\n", edge_img.cols / 2 - lane_center, lane_right_start_x, lane_left_start_x);
    }

    gettimeofday(&now, NULL);
    unsigned long end_time = now.tv_sec * 1000 + now.tv_usec / 1000;

    printf("Processing took: %lu msec\n", end_time - start_time);

    // write out images for analysis
    cv::imwrite(string("blur_img.jpg"), img_color);
    cv::imwrite(string("edge_img.jpg"), edge_img);

    return EXIT_SUCCESS;
}
