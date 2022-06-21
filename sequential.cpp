#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono>
#include "src/utils/file_writer.hpp"

using namespace std;
using namespace cv;

/**
 * @brief Multiply two matrixes pixel per pixel
 *
 * @param a Mat to multiply
 * @param b Mat to multiply
 * @return A matrix where the pixel i,j is the multiplication of pixel i,j of a and pixel i,j of b.
 */
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

/**
 * @brief Computes a pixel of the matrix with filter applied.
 * 
 * @param sub matrix to apply the filter to
 * @param filter filter matrix
 * @return The central pixel of the matrix with filter applied.
 */
float smoothing_px(Mat sub, Mat filter) {
    //Mat mul = sub.mul(h1);
    Mat mul = pixel_mul(sub, filter);
    float * p = (float *) mul.data;
    float res = 0;
    for(int i=0; i<mul.rows; i++) {
        for (int j=0; j<mul.cols; j++) {
            res += p[i * mul.cols + j];
        }
    }
    return res;
}

/**
 * @brief Performs smoothing of a matrix given a filter.
 * 
 * @param m matrix to filter
 * @param filter filter matrix
 * @param show flag to show the result matrix
 * @param times flag to show the execution time
 * @return The matrix m with smoothing filter applied
 */
Mat smoothing(Mat m, Mat filter, bool show, bool times) {
    float * gp = (float *) m.data;
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<m.rows; i++) {
        for (int j=0; j<m.cols; j++) { 
            // Create a rectangle with the bounds of the submatrix 3x3
            cv::Rect r(j-1, i-1, 3, 3);
            // Check that the bounds don't exceed the matrix bounds
            if (r.x >= 0 && r.y >= 0 && r.x + r.width <= m.cols && r.y + r.height <= m.rows) {
                Mat submatrix = m(r).clone();
                gp[i * m.cols + j] = smoothing_px(submatrix, filter);
            }
        }
    }
    if (times) {
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed for smoothing: " << usec << " usec" << endl;
    }
    if (show) {
        imshow("Smoothing", m);
        waitKey(25);
    }
    return m;
}

/**
 * @brief Converts a matrix from colors to black and white
 * 
 * @param frame matrix to convert
 * @param show flag to show the result matrix
 * @param times flag to show the execution time
 * @return Mat converted to grayscale
 */
Mat greyscale_conversion(Mat frame, bool show, bool times) {
    Mat gr = Mat(frame.rows, frame.cols, CV_32F);
    float * p = (float *) frame.data;
    float r, g, b;
    float * gp = (float *) gr.data;
    int channels = frame.channels();
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<frame.rows; i++) {
        for (int j=0; j<frame.cols; j++) {
            r = (float) p[i * frame.cols * channels + j * channels];
            g = (float) p[i * frame.cols * channels + j * channels + 1];
            b = (float) p[i * frame.cols * channels + j * channels + 2];
            gp[i* frame.cols + j ] = (float) (r + g + b) / channels;
        }
    }
    if (times) {
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed to convert to greyscale: " << usec << " usec" << endl;
    }
    if (show) {
        imshow("Frame", gr);
        waitKey(25);
    }
    return gr;
}


/**
 * @brief Performs background subtraction
 * 
 * @param frame frame to subtract to background
 * @param back background matrix
 * @param threshold threshold for background subtraction
 * @param show flag to show the result matrix
 * @param times flag to show the execution time
 * @return the fraction of different pixels between the background and the actual frame over the total
 */
float different_pixels(Mat frame, Mat back, float threshold, bool show, bool times) {
    int cnt = 0;
    Mat res = Mat(frame.rows, frame.cols, CV_32F);
    float * pa = (float *) frame.data;
    float * pb = (float *) back.data;
    float * pres = (float *) res.data;
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<frame.rows; i++) {
        for (int j=0; j<frame.cols; j++) {
            pres[i * frame.cols + j] = abs(pb[i * frame.cols + j] - pa[i * frame.cols + j]);
            if (pres[i * frame.cols + j] > threshold) cnt++;
        }
    }

    if (times) {
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed to compare frame with background: " << usec << " usec" << endl;
    }
    if (show) {
        imshow("Background subtraction", res);
        waitKey(25);
    }
    float diff_fraction = (float) cnt / frame.total();
    return diff_fraction;
}

// Sequential pattern
int main(int argc, char * argv[]) {

    // Parsing of the program arguments
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename k show" << endl;
        return 0;
    }
    bool show = false;
    bool times = false;
    if (argc == 4) {
        if (strcmp(argv[3], "show") == 0) show = true;
        else if (strcmp(argv[3], "times") == 0) times = true;
    }
    string output_file = "resultst/results.txt";
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i],"-output_file") == 0) {
            output_file = argv[i+1];
            break;
        }
    }
    string program_name = argv[0];
    program_name = program_name.substr(2, program_name.length()-1);
    string filename = argv[1];
    VideoCapture cap(filename);
    int k = atoi(argv[2]); // k for accuracy then
    float percent = (float) k / 100; // percentage of different pixels to esceed to detect a movement

    cout << "Sequential implementation" << endl;

    // Filter matrix for smoothing
    Mat h1 = Mat::ones(3, 3, CV_32F);
    // For smoothing it is used average filtering
    h1 = (Mat_<float>) (1/9 * h1);
    float * h1p = (float *) h1.data;
    for (int i=0; i<h1.rows; i++) {
        for (int j=0; j<h1.rows; j++) {
            *h1p++ = (float) 1/9;
        }
    }

    int frame_number = 0;
    int different_frames = 0;
    // Threshold to exceed to consider two pixels different
    float threshold = 0;

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    // The first frame is used as background image
    Mat background; 

    while(true) {
        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);

        // Greyscale conversion
        frame = greyscale_conversion(frame, show, times);
        
        // Smoothing
        frame = smoothing(frame, h1, show, times);

        if (frame_number == 0) { // Case first frame taken as background
            background = frame;
            // Get the average pixel intensity of the background to create a threshold
            float sum = 0;
            float * gp = (float *) frame.data;
            for(int i=0; i<frame.rows; i++) {
                for (int j=0; j<frame.cols; j++) {
                    sum = sum + gp[i* frame.cols + j];
                }
            }
            float avg_intensity = (float) sum / frame.total();
            avg_intensity = round( avg_intensity * 100.0 ) / 100.0;
            threshold = avg_intensity / 3;
            cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
            cout << "Background average intensity: " << avg_intensity << endl;
        }
        else { // Case movement detection
            float different_pixels_fraction = different_pixels(frame, background, threshold, show, times);
            if (different_pixels_fraction > percent) different_frames++;
            cout << "Frames with movement detected until now: " << different_frames << " over " << frame_number << " analyzed" << endl;
        }
        frame_number++;
    }
    cap.release();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();

    cout << "Number of frames with movement detected: " << different_frames << endl;
    cout << "Total time passed: " << complessive_usec << endl;
    
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, -1, show, time, different_frames);

    return(0);
}