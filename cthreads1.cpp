#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <atomic>

#include <future>
#include <optional>

using namespace std;
using namespace cv;

Mat convert_to_greyscale(Mat m, int nw) {
    Mat g = Mat(m.rows, m.cols, CV_32F);
    int rows = m.rows;
    int columns = m.cols;
    int size = rows*columns;
    int chunk_rows = rows/nw;
    int chunk_cols = columns/nw;
    int channels = m.channels();
    //vector<Mat> chunks(nw);
    vector<thread*> tids(nw);
    vector<pair<int,int>> chunks(nw);
    for (int i=0; i<nw; i++) {
        auto start = i*chunk_rows; 
        auto stop = (i==(nw-1) ? g.rows : start + chunk_rows);
        chunks[i] = make_pair(start,stop);
    }
    auto greyscale_conversion = [&g, rows, columns, channels, chunks, m] (int ti) {
        int first = chunks[ti].first;
        int final = chunks[ti].second;
        //cout << "Thread " << ti << " takes rows from " << first << " to " << final << endl;
        float * pl = (float *) g.data;
        float * pm = (float *) m.data;
        float r = 0;
        float b = 0;
        float g = 0;
        for(int i=first; i<final; i++) {
            for (int j=0; j<columns; j++) {
                r = (float) pm[i * columns * channels + j * channels];
                g = (float) pm[i * columns * channels + j * channels + 1];
                b = (float) pm[i * columns * channels + j * channels + 2];
                pl[i * columns + j] = (float) (r + g + b) / channels;
            }
        }
        return 0;
    };
    for (int i=0; i<nw; i++) {
        tids[i] = new thread(greyscale_conversion, i);
    }
    for (int i=0; i<nw; i++) {
        tids[i]->join();
        //cout << "Greyscale conversion: thread " << i << " finished " << endl;
    }
    return g;
}

int non_zero_pixels(Mat a) {
    int cnt = 0;
    float * p = (float *) a.data;
    for(int i=0; i<a.rows; i++) {
        for (int j=0; j<a.cols; j++) {
            if (p[i * a.cols + j] != 0) cnt++;
        }
    }
    return cnt;
}

Mat subtract(Mat a, Mat b) {
    Mat res = Mat(a.rows, a.cols, CV_32F);
    float * pa = (float *) a.data;
    float * pb = (float *) b.data;
    float * pres = (float *) res.data;
    for(int i=0; i<a.rows; i++) {
        for (int j=0; j<a.cols; j++) {
            //if (pa[i * a.cols + j] < pb[i * a.cols + j]) {
            //    pres[i * a.cols + j] = 0;
            //}
            //else {
            pres[i * a.cols + j] = pa[i * a.cols + j] - pb[i * a.cols + j];
            //}
        }
    }
    return res;
}

Mat pixel_mul(Mat a, Mat b) {
    Mat res = Mat(a.rows, a.cols, CV_32F);
    float * pa = (float *) a.data;
    float * pb = (float *) b.data;
    float * pres = (float *) res.data;
    for(int i=0; i<a.rows; i++) {
        for (int j=0; j<a.cols; j++) {
            pres[i * a.cols + j] = pa[i * a.cols + j] * pb[i * a.cols + j];
        }
    }
    return res;
}

float smoothing_px(Mat sub, Mat h1) {
    //Mat mul = sub.mul(h1);
    Mat mul = pixel_mul(sub, h1);
    float * p = (float *) mul.data;
    float res = 0;
    for(int i=0; i<mul.rows; i++) {
        for (int j=0; j<mul.cols; j++) {
            res += p[i * mul.cols + j];
        }
    }
    return res;
}

int main(int argc, char * argv[]) {
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename accuracy nw" << endl;
    }

    // Initialization
    string filename = argv[1];
    int k = atoi(argv[2]); // k for accuracy then
    int nw = atoi(argv[3]); // number of workers
    int uw = 2;
    int gw = (nw - uw)/3;
    int cw = (nw - uw)/3;
    int sw = nw - gw - cw - uw;
    cout << "Thread use: " << gw << " for conversion to greyscale, " << sw << " for smoothing and " << cw << " for comparing " << endl;
    // Start time measurement
    auto complessive_time_start = std::chrono::high_resolution_clock::now();
    float threshold = (float) k / 100;
    VideoCapture cap(filename); 
    Mat h1 = Mat::ones(3, 3, CV_32F); // Filter matrix for smoothing
    h1 = (Mat_<float>) (1/9 * h1);
    float * h1p = (float *) h1.data;
    for (int i=0; i<h1.rows; i++) {
        for (int j=0; j<h1.rows; j++) {
            *h1p++ = (float) 1/9;
        }
    }
    atomic<int> frame_number;
    atomic<int> different_frames;
    frame_number = 0;
    different_frames = 0;
    // Take first frame as background
    Mat current;
    Mat background; // The first frame is used as background image
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);

    mutex lg;
    mutex ls;
    mutex lc;
    mutex lr;
    deque<function<Mat()>> greyscale_tasks;
    deque<function<Mat()>> smoothing_tasks;
    deque<function<int()>> comparing_tasks;
    deque<float> results;
    condition_variable cond_greyscale;
    condition_variable cond_smoothing;
    condition_variable cond_comparing;
    condition_variable cond_results;
    bool stop = false;

    auto submit_greyscale = [&] (function<Mat()> f) {
        // section locked
        { 
            unique_lock<mutex> lock(lg);
            greyscale_tasks.push_back(f);
        }
        cond_greyscale.notify_one();
    };

    auto submit_smoothing = [&] (function<Mat()> f) {
        { 
            unique_lock<mutex> lock(ls);
            smoothing_tasks.push_back(f);
        }
        cond_smoothing.notify_one();
    };

    auto submit_comparing = [&] (function<int()> f) {
        { 
            unique_lock<mutex> lock(lc);
            comparing_tasks.push_back(f);
        }
        cond_comparing.notify_one();
    };

    auto submit_result = [&] (float res) {
        {
            unique_lock<mutex> lock(lr);
            results.push_back(res);
        }
        cond_results.notify_one();
    };

    auto smoothing = [&] (Mat m) {
        auto start = std::chrono::high_resolution_clock::now();
        float * sp = (float *) m.data;
        for(int i=0; i<m.rows; i++) {
            for (int j=0; j<m.cols; j++) { 
                cv::Rect r(j-1, i-1, 3, 3);
                if (r.x >= 0 && r.y >= 0 && r.x + r.width <= m.cols && r.y + r.height <= m.rows) {
                    Mat submatrix = m(r).clone();
                    //*sp++ = smoothing_px(submatrix, h1);
                    sp[i * m.cols + j] = smoothing_px(submatrix, h1);
                }
            }
        }
        //imshow("Smoothing", m);
        //waitKey(25);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed in seconds for smoothing: " << usec << endl;
        return m;
    };

    auto different_pixels = [&] (Mat m) {
        auto start = std::chrono::high_resolution_clock::now();
        int rows = m.rows;
        int columns = m.cols;
        int size = rows*columns;
        int chunk_rows = rows/cw;
        int chunk_cols = columns/cw;
        int channels = m.channels();
        //vector<Mat> chunks(nw);
        vector<thread*> tids(cw);
        vector<pair<int,int>> chunks(nw);
        for (int i=0; i<nw; i++) {
            auto start = i*chunk_rows; 
            auto stop = (i==(cw-1) ? m.rows : start + chunk_rows);
            chunks[i] = make_pair(start,stop);
        }
        atomic<int> different_pixels;
        different_pixels = 0;
        Mat res = Mat(m.rows, m.cols, CV_32F);
        float * pa = (float *) m.data;
        float * pb = (float *) background.data;
        float * pres = (float *) res.data;
        auto comparing = [&] (int ti) {
            int first = chunks[ti].first;
            int final = chunks[ti].second;
            for(int i=first; i<final; i++) {
                for (int j=0; j<columns; j++) {
                    pres[i * m.cols + j] = abs(pb[i * m.cols + j] - pa[i * m.cols + j]);
                    if (pres[i * m.cols + j] > 0) different_pixels++;
                }
            }
            //imshow("Subtraction", res);
            //waitKey(25);
        };
        for (int i=0; i<cw; i++) {
            tids[i] = new thread(comparing, i);
        }
        for (int i=0; i<cw; i++) {
            tids[i]->join();
        }
        float diff_pixels_fraction = (float) different_pixels/m.total();
        submit_result(diff_pixels_fraction);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed in seconds to compare frame with background: " << usec << endl;
        return 0;
    };

    auto comparing_body = [&] () {
        
        function<int()> t = []() { return 0;};
        while (!stop) {
            {
                unique_lock<mutex> lock(lc);
                cond_comparing.wait(lock, [&](){return(!comparing_tasks.empty() || stop);});
                if (!comparing_tasks.empty()) {
                    t = comparing_tasks.front();
                    comparing_tasks.pop_front();
                }
                if (stop){
                    return 1;
                } 
            }
            t();
            //if (stop && comparing_tasks.empty()) {
            //    return 1;
            //}
        }
        
        return 0;
    };

    auto smoothing_body = [&] (int i) {
        function<Mat()> t = []() {return Mat::ones(2, 2, CV_32F);};
        while (!stop || !stop) {
            {
                unique_lock<mutex> lock(ls);
                cond_smoothing.wait(lock, [&](){return(!smoothing_tasks.empty() || stop);});
                if (!smoothing_tasks.empty()) {
                    t = smoothing_tasks.front();
                    smoothing_tasks.pop_front();
                }
                if (stop){
                    return;
                } 
            }
            //cout << "Thread " << i << " works for smoothing" << endl;
            Mat s = t();
            //imshow("Smoothing", s);
            //waitKey(25);
            auto f = (bind(different_pixels, s));
            submit_comparing(f);
            //if (stop && smoothing_tasks.empty()) {
                //return;
            //}
        }
        return;
    };

    vector<thread> tids(sw);
    thread comparing(comparing_body);
    for (int i=0; i<sw; i++) {
        //cout << "Threads " << i << " for smoothing started" << endl;
        tids[i] = thread(smoothing_body, i);
    }

    // Background preparation
    background = convert_to_greyscale(background, gw);
    background = smoothing(background);

    while(true) {

        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);

        // GREYSCALE CONVERSION
        auto start = std::chrono::high_resolution_clock::now();
        Mat gr = convert_to_greyscale(frame, gw);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed in seconds to convert to greyscale: " << usec << endl;
        //imshow("Greyscale", gr);
        //waitKey(25);

        auto f = (bind(smoothing, gr));
        submit_smoothing(f);

        frame_number++;

    }

    cap.release();

    int res_number = 0;
    float res = 0;
    while (res_number <= frame_number) {
        {
            unique_lock<mutex> lock(lr);
            cond_results.wait(lock, [&](){return(!results.empty());});
            if (!results.empty()) {
                res = results.front();
                results.pop_front();
            }
            if (res_number == frame_number) break;
        }
        if (res > threshold) different_frames++;
        res_number++;
        cout << "Frames with movement detected until now: " << different_frames << " over " << res_number << " analyzed on a total of " << frame_number << endl;
        if (res_number == frame_number) break;
    }

    stop = true;
    cond_comparing.notify_all();
    cond_greyscale.notify_all();
    cond_smoothing.notify_all();
    
    for (int i=0; i<sw; i++) {
        tids[i].join();
    }
    //cout << "joining comparing" << endl;
    comparing.join();
    cout << "Number of frames with movement detected: " << different_frames << endl;
    auto complessive_time_end = std::chrono::high_resolution_clock::now();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time passed: " << complessive_usec << endl;

    return 0;
}