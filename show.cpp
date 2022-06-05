#include <iostream>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

void activewait(std::chrono::milliseconds ms) {
    long msecs = ms.count();
    auto start = std::chrono::high_resolution_clock::now();
    auto end = false;
    while (!end) {
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        if (msec>msecs) end = true;
    }
    return;
}

// compile with (maybe some flags are missing): g++ program.cpp -o program `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

int main(int argc, char * argv[]) {
    VideoCapture cap(argv[1]);
    while(true) {
        Mat frame; 
        cap >> frame;
        if(frame.empty())
        break;
        //cout << "M = " << endl << " " << frame << endl << endl;
        //imshow("Prova", frame );
        //waitKey(25);
    }
    cap.release();
    return(0);
}