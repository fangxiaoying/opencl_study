#include <opencv2/opencv.hpp>
#include <string>
#include <sstream>


using namespace std;


#define BILLION      1000000000
uint64_t get_perf_count();

uint64_t sigStart, sigEnd;
float msVal;
int fps;

/*
base line.  
just use opencv read and show camera data.

enviroment:
imx8mp, L5.15.5_1.0.0
show video size 1080p

performance:
fps 12
cup loading:
open_camera_0 : 103 %
weston: 9 %
*/
 
int main()
{
	//读取视频或摄像头
	cv::VideoCapture capture(3);
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
 
	while (true)
	{
		cv::Mat frame;
        string str = "test";


        sigStart = get_perf_count();
		capture >> frame;

        cv::putText(frame, str, cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255,230,0), 2);

        cv::imshow("read video", frame);
		cv::waitKey(30);

        sigEnd = get_perf_count();
        msVal = (sigEnd - sigStart)/1000000;

        fps = 1000 / msVal;
        string fps_info = "to_string (fps);";
        printf("time: %.2fms , fps = %d \n", msVal,fps);

	}
	return 0;

}

uint64_t get_perf_count()
{
#if defined(__linux__) || defined(__ANDROID__) || defined(__QNX__) || defined(__CYGWIN__)
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)((uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * BILLION);
#elif defined(_WIN32) || defined(UNDER_CE)
    LARGE_INTEGER ln;

    QueryPerformanceCounter(&ln);

    return (uint64_t)ln.QuadPart;
#endif
}