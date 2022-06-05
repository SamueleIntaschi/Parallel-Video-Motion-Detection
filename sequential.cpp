#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 

using namespace std;
using namespace cv;

// compile with (maybe some flags are missing): g++ program.cpp -o program `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

// Sequential pattern
int main(int argc, char * argv[]) {
  if (argc == 1) {
    cout << "Usage is " << argv[0] << " filename accuracy" << endl;
  }

  // Initialization
  double complessive_time = (double)getTickCount();
  VideoCapture cap(argv[1]);
  int k = atoi(argv[2]); // k for accuracy then
  float accuracy = (float) k / 100;
  // Matrix for smoothing
  Mat h1 = Mat::ones(3, 3, CV_32F);
  h1 = (Mat_<float>) (1/9 * h1);
  float * h1p = (float *) h1.data;
  for (int i=0; i<h1.rows; i++) {
    for (int j=0; j<h1.rows; j++) {
      *h1p++ = (float) 1/9;
    }
  }
  int frame_number = 0;
  int different_frames = 0;
  // Matrixes for make comparison between previous and current frame
  Mat previous;
  Mat current;

  while(true) {
    Mat frame; 
    cap >> frame;
    if(frame.empty())
    break;
    frame.convertTo(frame, CV_32F, 1.0/255.0);
    // Greyscale conversion
    int channels = frame.channels();
    int rows = frame.rows;
    //int columns = frame.cols * channels;
    int columns = frame.cols;
    //cout << "The matrix is of type " << type << ", has " << channels << " channels, " << rows << " rows and " << columns << " columns" << endl;
    Mat greyscale = Mat(rows, columns, CV_32F);
    float * p = (float *) frame.data;
    float r, g, b;
    float * gp = (float *) greyscale.data;
    double t = (double)getTickCount();
    for(int i=0; i<rows; i++) {
      for (int j=0; j<columns; j++) {
        r = (float) p[i * columns * channels + j * channels];
        g = (float) p[i * columns * channels + j * channels + 1];
        b = (float) p[i * columns * channels + j * channels + 2];
        //cout << r << " " << g << " " << b << endl;
        *gp++ = (float) (r + g + b) / channels;
      }
    }

    t = ((double)getTickCount() - t)/getTickFrequency();
    cout << "Times passed in seconds to convert to greyscale: " << t << endl;

    // Smoothing
    Mat smoothed = Mat(greyscale.rows, greyscale.cols, CV_32F);
    float * sp = (float *) smoothed.data;
    t = (double)getTickCount();
    for(int i=0; i<greyscale.rows; i++) {
      for (int j=0; j<greyscale.cols; j++) { 
          // TODO errore qui
          cv::Rect r(i-1, j-1, 3, 3);
          if (r.x >= 0 && r.y >= 0 && r.x + r.width <= greyscale.cols && r.y + r.height <= greyscale.rows) {
            Mat submatrix = greyscale(r).clone();
            Mat res = submatrix * h1;
            *sp++ = res.at<float>(2,2);
          }
          else {
            *sp++ = greyscale.at<float>(i,j);
          }
      }
    }
    t = ((double)getTickCount() - t)/getTickFrequency();
    cout << "Times passed in seconds to convert for smoothing: " << t << endl;
    //imshow("Smoothed", smoothed);
    //waitKey(25);

    // Movement detection
    int different_pixels = 0;
    current = smoothed;
    t = (double)getTickCount();
    if (frame_number > 0) {
      //float * pp = (float *) previous.data;
      //float * cp = (float *) current.data;
      for (int i=0; i<smoothed.rows; i++) {
        for (int j=0; j<smoothed.cols; j++) {
          if (previous.at<float>(i,j) != current.at<float>(i,j)) different_pixels++;
        }
      }
      float total_pixels = current.total();
      float different_pixels_fraction  = (float) different_pixels / total_pixels;
      //cout << "Different pixels between previous frame: " << different_pixels_fraction << " for frame number " << frame_number << endl;
      if (different_pixels_fraction > accuracy) different_frames++;
    }
    t = ((double)getTickCount() - t)/getTickFrequency();
    cout << "Times passed in seconds to check movements: " << t << endl;
    frame_number++;
    previous = smoothed;
    cout << "Frame number " << frame_number << " -> number of frames with movement detected until now: " << different_frames << endl;
  }
  cout << "Number of frames with movement detected: " << different_frames << endl;
  cap.release();
  complessive_time = ((double)getTickCount() - complessive_time)/getTickFrequency();
  cout << "Complessive time:  " << complessive_time << endl;
  return(0);
}