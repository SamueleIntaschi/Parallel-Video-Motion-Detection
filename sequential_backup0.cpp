#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 

using namespace std;
using namespace cv;

// compile with (maybe some flags are missing): g++ program.cpp -o program `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

// Sequential pattern

// Returns 0 for unsigned int, 1 for signed integers, 2 for float, 3 if not found
/*
data	uchar	Uint8Array	CV_8U
data8S	char	Int8Array	CV_8S
data16U	ushort	Uint16Array	CV_16U
data16S	short	Int16Array	CV_16S
data32S	int	Int32Array	CV_32S
data32F	float	Float32Array	CV_32F
data64F	double	Float64Array	CV_64F

*/
int opencv_mat_type(int depth) {
  switch ( depth ) {
    case CV_8U:  return 0;
    case CV_8S:  return 1;
    case CV_16U: return 0;
    case CV_16S: return 1;
    case CV_32S: return 1;
    case CV_32F: return 2;
    case CV_64F: return 2;
    default:     return 3;
  }
}

string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

int main(int argc, char * argv[]) {
  if (argc == 1) {
    cout << "Usage is " << argv[0] << " filename accuracy" << endl;
  }
  double complessive_time = (double)getTickCount();
  VideoCapture cap(argv[1]);
  int k = atoi(argv[2]); // k for accuracy then
  // Matrix for smoothing
  Mat h1 = Mat::ones(3, 3, CV_32F);
  h1 = (Mat_<float>) (1/9 * h1);
  float * h1p = (float *) h1.data;
  for (int i=0; i<h1.rows; i++) {
    for (int j=0; j<h1.rows; j++) {
      *h1p++ = (float) 1/9;
    }
  }
  //cout << h1 << endl;
  int frame_number = 0;
  int different_frames = 0;
  Mat previous;
  while(true) {
    Mat frame; 
    cap >> frame;
    if(frame.empty())
    break;
    frame.convertTo(frame, CV_32F, 1.0/255.0);
    //imshow("Prova", frame );
    //waitKey(25);

    /*
      TODO: Convert to greyscale

            Converting an image to greyscale can be achieved by 
            substituting each (R,G,B) pixel with a grey pixel with a grey value which is the average of the 
            R, G and B values.
    */
    int channels = frame.channels();
    string type = type2str(frame.type());
    int rows = frame.rows;
    //int columns = frame.cols * channels;
    int columns = frame.cols;
    //cout << "The matrix is of type " << type << ", has " << channels << " channels, " << rows << " rows and " << columns << " columns" << endl;
    Mat greyscale = Mat(rows, columns, CV_32F);
    /*
    if (frame.isContinuous()) {
      //cout << "Storage is continuous" << endl;
      //columns = columns * rows;
      //rows = 1;
    }
    else {
      //cout << "Storage is not continuous" << endl;
    }
    */
    float * p = (float *) frame.data;
    float r, g, b;
    float * gp = (float *) greyscale.data;
    //int cnt = 0;
    //uchar sum;
    double t = (double)getTickCount();
    //int size = columns * channels * rows;

    /*
    for (int i=0; i<size; i++) {
      sum += p[i];
      if (i % channels == 0) {
        *gp++ = sum / channels;
        sum = 0;
        cnt = 0;
      }
      else cnt++;
    }
    */

    /*
    vector<uchar> ps(channels);
    for (int i=0; i<rows; i++) {
      for (int j=0; j<columns; j++) {
        for (int z=0; z<channels; z++) {
          r = p[i * columns * channels + j * channels + z];
          sum += r;
          if (z == channels - 1) {
            *gp++ = sum / channels;
            sum = 0;
          }
        }
      }
    }
    */
    
    /*
    for(int i=0; i<rows; i++) {
      for (int j=0; j<columns; j++) {
        r = p[i * columns * channels + j * channels + cnt];
        sum = sum + r;
        if (cnt == channels - 1) {
          *gp++ = sum / channels;
          cnt = 0;
          sum = 0;
        }
        else cnt++;
      }
    }
    */

    /*
    columns = columns * channels;
    for(int i=0; i<rows; i++) {
      p = frame.ptr<uchar>(i);
      for (int j=0; j<columns; j++) {
        sum += p[j];
        if (cnt == channels - 1) {
          *gp++ = (r + g + b) / channels;
          sum = 0;
          cnt = 0;
        }
        else cnt++;
      }
    }
    */

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
    //imshow("Greyscale", greyscale );
    //waitKey(25);
  
    /*
      TODO: Smoothing

            Smoothing requires pixel is obtained “averaging” its value with the value of the surrounding pixels. 
            This is achieved by usually multiplying the 3x3 pixel neighborhood by one of the matrixes
            and subsequently taking as the new value of the pixel the central value in the resulting matrix 
            (H1 computes the average, the other Hi computes differently weighted averages.
    */
   
    //Mat gs2 = greyscale.clone();
    Mat smoothed = Mat(greyscale.rows, greyscale.cols, CV_32F);
    float * sp = (float *) smoothed.data;
    //cout << h1 << endl;
    t = (double)getTickCount();
    for(int i=0; i<greyscale.rows; i++) {
      for (int j=0; j<greyscale.cols; j++) { 
          // TODO errore qui
          cv::Rect r(i-1, j-1, 3, 3);
          if (r.x >= 0 && r.y >= 0 && r.x + r.width <= greyscale.cols && r.y + r.height <= greyscale.rows) {
            Mat submatrix = greyscale(r).clone();
            //TODO realizzare media con moltiplicazione matrici come prima
            /*
            float msum = 0;
            for (int z=0; z<submatrix.rows; z++) {
              for (int q=0; q<submatrix.cols; q++) {
                msum += greyscale.at<float>(i,j);
              }
            }
            float mean = msum / 9;
            *sp++ = mean;
            */
            Mat res = submatrix * h1;
            *sp++ = res.at<float>(2,2);
            //cout << res.at<float>(2,2) << ", " << mean << endl;
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

    /*
      TODO: 1. The first frame of the video is taken as “background picture”
            2. Each frame is turned to greyscale, smoothed, and subtracted to the greyscale smoothed background picture
            3. Motion detect flag will be true in case more the k% of the pixels differ
            4. The output of the program is represented by the number of frames with motion detected
    */

    int different_pixels = 0;
    Mat current = smoothed;
    t = (double)getTickCount();
    if (frame_number > 0) {
      //float * pp = (float *) previous.data;
      //float * cp = (float *) current.data;
      for (int i=0; i<smoothed.rows; i++) {
        for (int j=0; j<smoothed.cols; j++) {
          if (previous.at<float>(i,j) != current.at<float>(i,j)) different_pixels++;
        }
      }
      if ((different_pixels / (rows*columns)) > (k / 100)) different_frames++;
    }
    t = ((double)getTickCount() - t)/getTickFrequency();
    cout << "Times passed in seconds to check movements: " << t << endl;
    frame_number++;
    previous = smoothed;
  }
  cout << "Number of frames with movement: " << different_frames << endl;
  cap.release();
  cout << "Complessive time:  " << complessive_time << endl;
  return(0);
}