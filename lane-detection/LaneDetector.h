#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <sys/time.h>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

struct Detection
{
    int center_offset; ///< offset from center in pixels
    double heading;    ///< deviation from straight in radians
    double confidence; ///< confidence that a lane has actually be detected
};

class LaneDetector
{
private:


public:
    struct Detection detect_vehicle_pose(cv::Mat& img);
};
