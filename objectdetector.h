#ifndef OBJECTDETECTOR_H
#define OBJECTDETECTOR_H

#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "dbscan.h"

using namespace std;

class ObjectDetector{

public:
    ObjectDetector(vector<cv::Mat>& imageSet, double threshold, unsigned int clusterSize, const cv::Mat& mask);
    ObjectDetector(double threshold, unsigned int clusterSize, const cv::Mat& mask);
    ~ObjectDetector();

    vector<cv::Rect> evaluate(cv::Mat& image, cv::Mat& output);
    void         addToBaseline(cv::Mat& image);
    void         clearBaseline();
    unsigned int getImageSetSize();

    double       getThreshold();
    unsigned int getClusterSize();
    double       getEpsilon();

    void setThreshold(double threshold);
    void setClusterSize(unsigned int clusterSize);
    void setEpsilon(double epsilon);

private:
    vector<cv::Mat> imageSet;
    double m_scale, m_invscale, m_threshold, m_epsilon;
    unsigned int m_totalTraining, m_clusterSize;
    int m_rh, m_rw, m_horizontal, m_vertical;

    cv::Mat m_mask;
    DBSCAN* dbscan;
    cv::Mat buildBaseline(vector<cv::Mat>& imageSet, cv::Mat& image);
};

#endif // OBJECTDETECTOR_H
