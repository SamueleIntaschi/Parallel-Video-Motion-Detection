#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono>
#include "src/utils/file_writer.hpp"

using namespace std;
using namespace cv;

/**
 * @brief Performs smoothing of a matrix given a filter.
 * 
 * @param m matrix to filter
 * @param filter filter matrix
 * @param show flag to show the result matrix
 * @param times flag to show the execution time
 * @return The matrix m with smoothing filter applied
 */
Mat smoothing(Mat m, bool show) {
    Mat res = Mat(m.rows, m.cols, CV_32F, 0.0);
    float * gp = (float *) m.data;
    float * mp = (float *) res.data;
    for(int i=0; i<m.rows; i++) {
        for (int j=0; j<m.cols; j++) { 
            int x = j-1;
            int y = i-1;
            int width = 3;
            int height = 3;
            if (x >= 0 && y >= 0 && x + width < m.cols && y + height < m.rows) {
                for(int z=y; z<y+height; z++) {
                    for (int k=x; k<x+width; k++) {
                        mp[i * res.cols + j] = mp[i * res.cols + j] + (float) (gp[z * m.cols + k] / 9);
                        //res.at<float>(i,j) = res.at<float>(i,j) + (float) (m.at<float>(z,k)/9);
                    }
                }
            }
            else {
                mp[i * res.cols + j] = gp[i * res.cols + j];
            }
        }
    }
    if (show) {
        imshow("Smoothing", res);
        waitKey(25);
    }
    return res;
}

/**
 * @brief Converts a matrix from colors to black and white
 * 
 * @param frame matrix to convert
 * @param show flag to show the result matrix
 * @param times flag to show the execution time
 * @return Mat converted to grayscale
 */
Mat greyscale_conversion(Mat frame, bool show) {
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
float different_pixels(Mat frame, Mat back, float threshold, bool show) {
    int cnt = 0;
    float * pa = (float *) frame.data;
    float * pb = (float *) back.data;
    for(int i=0; i<frame.rows; i++) {
        for (int j=0; j<frame.cols; j++) {
            float difference = (float) abs(pb[i * frame.cols + j] - pa[i * frame.cols + j]);
            pa[i * frame.cols + j] = difference;
            if (difference > threshold) cnt++;
        }
    }
    if (show) {
        imshow("Background subtraction", frame);
        waitKey(25);
    }
    float diff_fraction = (float) cnt / frame.total();
    return diff_fraction;
}

/**
 * @brief Get the avg time of the times stored in a vector
 * 
 * @param usecs vector that stores the times
 * @return auto average value
 */
auto get_avg_time(vector<chrono::microseconds> usecs) {
    int size = usecs.size();
    chrono::microseconds sum = 0ms;
    for (int i=0; i<size; i++) {
        sum += usecs[i];
    }
    return (chrono::microseconds) sum/size;
}

/**
 * @brief Print how to use the program
 * 
 * @param prog the name of the program
 */
void print_usage(string prog) {
    cout << "Basic usage is " << prog << " filename k" << endl;
    cout << "Options are: \n" <<
    "-info: shows times information \n" <<
    "-show: shows results frames for each stage \n"
    << endl;
}

// Sequential pattern
int main(int argc, char * argv[]) {

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    // flag to show result frames for each phase
    bool show = false;
    // flag to show the time for each phase
    bool times = false;
    // Creation of the name of the file to write the results to
    string program_name = argv[0];
    program_name = program_name.substr(2, program_name.length()-1);
    // Name of the video
    string filename = argv[1];
    string output_file = "results/" + filename.substr(filename.find('/')+1, filename.length() - filename.find('/')-(filename.length() - filename.find('.')) - 1) + ".txt";
    
    // Options parsing
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-show") == 0) show = true;
        if (strcmp(argv[i], "-info") == 0) times = true;
        if (strcmp(argv[i], "-help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    // Percent of different pixels needed to detect a movement in a frame
    int k = atoi(argv[2]); // k for accuracy then
    float percent = (float) k / 100; // percentage of different pixels to esceed to detect a movement

    cout << "Sequential implementation" << endl;
    VideoCapture cap(filename);

    int frame_number = 0;
    int different_frames = 0;
    // Threshold to exceed to consider two pixels different
    float threshold = 0;

    // Vectors to save the execution time for each phase and compute the average values at the end
    vector<chrono::microseconds> gr_usecs; 
    vector<chrono::microseconds> sm_usecs; 
    vector<chrono::microseconds> cmp_usecs; 

    // The first frame is used as background image
    Mat background; 

    // Read the frames from the video and perform the actions
    while(true) {

        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);

        // Greyscale conversion
        auto start = std::chrono::high_resolution_clock::now();
        frame = greyscale_conversion(frame, show);
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        gr_usecs.push_back(chrono::microseconds(usec));
        if (times) cout << "Times spent on greyscale conversion: " << usec << " usec" << endl;
        
        // Smoothing
        start = std::chrono::high_resolution_clock::now();
        frame = smoothing(frame, show);
        duration = std::chrono::high_resolution_clock::now() - start;
        usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        sm_usecs.push_back(chrono::microseconds(usec));
        if (times) cout << "Times spent on smoothing: " << usec << " usec" << endl;

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
            threshold = avg_intensity / 10;
            cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
            cout << "Background average intensity: " << avg_intensity << endl;
        }
        else { // Case movement detection
            start = std::chrono::high_resolution_clock::now();
            float different_pixels_fraction = different_pixels(frame, background, threshold, show);
            if (different_pixels_fraction > percent) different_frames++;
            duration = std::chrono::high_resolution_clock::now() - start;
            usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            cmp_usecs.push_back(chrono::microseconds(usec));
            if (times) cout << "Times passed for background subtraction: " << usec << " usec" << endl;
            if (times) cout << "Frames with movement detected until now: " << different_frames << " over " << frame_number << " analyzed" << endl;
        }
        // Increment the number of total frames
        frame_number++;
    }

    cap.release();

    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();

    cout << "Number of frames with movement detected: " << different_frames << endl;
    cout << "Average time spent for greyscale conversion: " << get_avg_time(gr_usecs).count() << endl;
    cout << "Average time spent for smoothing: " << get_avg_time(sm_usecs).count() << endl;
    cout << "Average time spent for background subtraction: " << get_avg_time(cmp_usecs).count() << endl;
    cout << "Total time passed: " << complessive_usec << endl;
    
    // Write the results in a file
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, -1, show, time, different_frames);

    return(0);
}