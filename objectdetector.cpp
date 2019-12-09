#include "objectdetector.h"
#include <wtypes.h>

const int BB_OFFSET          =   10;
const int FINAL_WIDTH        = 1920;
const int FINAL_HEIGHT       = 1080;
const int DOWNSAMPLED_HEIGHT =  216;
const int DOWNSAMPLED_WIDTH  =  384;
const int TITLE_BASELINE     =    1;
const int TITLE_SD           =    2;
const int TITLE_SUBTRACTED   =    3;
const int TITLE_MASKED       =    4;
const int TITLE_THRESHOLDED  =    5;

static cv::Mat menu, tBaseline, tSD, tSubtracted, tMasked, tThresholded;

static void GetDesktopResolution(int& horizontal, int& vertical){
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

ObjectDetector::ObjectDetector(vector<cv::Mat>& imageSet, double threshold, unsigned int clusterSize, const cv::Mat& mask)
    :m_threshold(threshold), m_clusterSize(clusterSize){
    if(imageSet.size() != 0) {
        this->m_totalTraining = imageSet.size();
        this->m_scale = static_cast<double>(DOWNSAMPLED_HEIGHT)/imageSet[0].size[0];
        this->m_invscale = 1.0/this->m_scale;
        for(unsigned int i = 0; i < this->m_totalTraining; i++){
            cv::Mat rsImage;
            resize(imageSet[i].clone(), rsImage, cv::Size(), this->m_scale, this->m_scale);
            this->imageSet.push_back(rsImage);
        }
        cv::resize(mask.clone(), this->m_mask, cv::Size(), this->m_scale, this->m_scale);
        cv::cvtColor(this->m_mask, this->m_mask, cv::COLOR_BGR2GRAY);
        this->m_epsilon = 8;
        this->m_rh = this->m_mask.size[0];
        this->m_rw = this->m_mask.size[1];
        this->dbscan = new DBSCAN(this->m_epsilon, this->m_clusterSize);
        this->m_horizontal = 0;
        this->m_vertical = 0;
        GetDesktopResolution(this->m_horizontal, this->m_vertical);
    }else{
        return;
    }
}

ObjectDetector::ObjectDetector(double threshold, unsigned int clusterSize, const cv::Mat& mask)
    :m_threshold(threshold), m_clusterSize(clusterSize){
    this->m_scale = static_cast<double>(DOWNSAMPLED_HEIGHT)/mask.size[0];
    this->m_invscale = 1.0/this->m_scale;
    cv::resize(mask.clone(), this->m_mask, cv::Size(), this->m_scale, this->m_scale);
    cv::cvtColor(this->m_mask, this->m_mask, cv::COLOR_BGR2GRAY);
    this->m_epsilon = 8;
    this->m_rh = this->m_mask.size[0];
    this->m_rw = this->m_mask.size[1];
    this->dbscan = new DBSCAN(this->m_epsilon, this->m_clusterSize);
    GetDesktopResolution(this->m_horizontal, this->m_vertical);

    menu         = cv::imread("images/menu.png", cv::IMREAD_UNCHANGED);
    tBaseline    = cv::imread("images/baseline.png", cv::IMREAD_UNCHANGED);
    tSD          = cv::imread("images/sd.png", cv::IMREAD_UNCHANGED);
    tSubtracted  = cv::imread("images/subtracted.png", cv::IMREAD_UNCHANGED);
    tMasked      = cv::imread("images/masked.png", cv::IMREAD_UNCHANGED);
    tThresholded = cv::imread("images/thresholded.png", cv::IMREAD_UNCHANGED);
}

double ObjectDetector::getThreshold(){
    return this->m_threshold;
}

unsigned int ObjectDetector::getClusterSize(){
    return this->m_clusterSize;
}

double ObjectDetector::getEpsilon(){
    return this->m_epsilon;
}

void ObjectDetector::setThreshold(double threshold){
       this->m_threshold = threshold;
}

void ObjectDetector::setClusterSize(unsigned int clusterSize){
    this->m_clusterSize = clusterSize;
    this->dbscan->setMinPoints(clusterSize);
}

void ObjectDetector::setEpsilon(double epsilon){
    this->m_epsilon = epsilon;
    this->dbscan->setEpsilon(epsilon);
}

unsigned int ObjectDetector::getImageSetSize(){
    return this->imageSet.size();
}

void ObjectDetector::addToBaseline(cv::Mat& image){
    cv::Mat rsImage;
    cv::resize(image.clone(), rsImage, cv::Size(), this->m_scale, this->m_scale);
    this->imageSet.push_back(rsImage);
    this->m_totalTraining = this->imageSet.size();
}

void ObjectDetector::clearBaseline(){
    this->imageSet.clear();
}

cv::Mat makeTitleBar(int title, cv::Mat image){
    cv::Mat titleImage;
    switch (title){
        case TITLE_BASELINE: titleImage = tBaseline;
            break;
        case TITLE_SD: titleImage = tSD;
            break;
        case TITLE_SUBTRACTED: titleImage = tSubtracted;
            break;
        case TITLE_MASKED: titleImage = tMasked;
            break;
        case TITLE_THRESHOLDED: titleImage = tThresholded;
            break;
    }

    cv::Mat newImage = image.clone();
    cv::Mat titleBlend;
    cv::Mat roi(newImage, cv::Rect(0, 0, image.size[1], BB_OFFSET * 4));
 cout << titleImage.size() << endl;
 cout << roi.size << endl;
    addWeighted(titleImage, 0.5, roi, 0.5, 0.0, titleBlend);
    titleBlend.copyTo(roi);
    cv::Rect r = cv::Rect(0, 0, DOWNSAMPLED_WIDTH, DOWNSAMPLED_HEIGHT);
    rectangle(newImage, r, cv::Scalar(255, 255, 255), 1, 0);
    return newImage;
}

vector<cv::Rect> ObjectDetector::evaluate(cv::Mat& image, cv::Mat& output){
    vector<cv::Rect> BB;
    if(this->imageSet.size() == 0){
        cv::Mat outputScreen(FINAL_HEIGHT, FINAL_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::Mat rsim;
        cv::resize(image, rsim, cv::Size(FINAL_WIDTH, FINAL_HEIGHT), 0, 0);
        cv::Mat roi(outputScreen, cv::Rect(0, 0, rsim.cols, rsim.rows));
        rsim.copyTo(roi);
        output = outputScreen;
        return BB;
    }

    cv::Mat rsImage;
    cv::resize(image, rsImage, cv::Size(), this->m_scale, this->m_scale);
    cv::Mat sumImage(this->m_rh, this->m_rw, CV_32FC3, cv::Scalar(0, 0, 0));
    cv::Mat temp;
    for (unsigned int i = 0; i < this->m_totalTraining; i++){
        this->imageSet[i].convertTo(temp, CV_32FC3);
        sumImage += temp;
    }

    temp = sumImage/this->m_totalTraining;
        cv::Mat meanImage;
        meanImage = temp.clone();

        cv::Mat baseline = buildBaseline(this->imageSet, rsImage);

        cv::Mat baselineF;
        baseline.convertTo(baselineF, CV_32FC3);

        cv::Mat rsImageF;
        rsImage.convertTo(rsImageF, CV_32FC3);

        cv::Mat diffImage = abs(rsImageF - baselineF);
        cv::Mat varImage(this->m_rh, this->m_rw, CV_32FC3, cv::Scalar(0, 0, 0));

        for (unsigned int i = 0; i < this->m_totalTraining; i++){
            this->imageSet[i].convertTo(temp, CV_32FC3);
            cv::Mat diff = temp - meanImage;
            cv::Mat square;
            pow(diff, 2, square);
            varImage += square;
        }

        cv::Mat stdImage;
            sqrt(varImage/this->m_totalTraining, stdImage);

            varImage = cv::Mat(this->m_rh, this->m_rw, CV_32FC3, cv::Scalar(0, 0, 0));

            for (unsigned int i = 0; i < this->m_totalTraining; i++){
                this->imageSet[i].convertTo(temp, CV_32FC3);
                cv::Mat diff = temp - baselineF;
                cv::Mat square;
                pow(diff, 2, square);
                varImage += square;
            }
            sqrt(varImage/this->m_totalTraining, stdImage);

            stdImage = stdImage * 2;
                cv::Mat outliersF = diffImage - stdImage;
                cv::Mat outliers;
                outliersF.convertTo(outliers, CV_8UC3);
                cv::Mat maskedOutliers;
                outliers.copyTo(maskedOutliers, m_mask/255);
                cv::Mat thImage(maskedOutliers.size[0], maskedOutliers.size[1], CV_8UC3, cv::Scalar(0, 0, 0));
                for (int i = 0; i < maskedOutliers.size[0]; i++){
                    for (int j = 0; j < maskedOutliers.size[1]; j++){
                        cv::Vec3b pixel = maskedOutliers.at<cv::Vec3b>(i, j);
                        if ((pixel.val[0] >= this->m_threshold * 255 || pixel.val[1] >= this->m_threshold * 255 || pixel.val[2] >= this->m_threshold * 255)){
                            thImage.at<cv::Vec3b>(i, j) = cv::Vec3b(255, 255, 255);
                        }
                    }
                }

                cv::Mat kernel(5, 5, CV_8U, cv::Scalar(1));
                cv::morphologyEx(thImage, thImage, cv::MORPH_CLOSE, kernel);

                cv::Mat rsThImage;
                cv::resize(thImage, rsThImage, cv::Size(), 0.25, 0.25);

                vector<pair<double, double>> dataset;
                int k = 0;
                for (int i = 0; i < rsThImage.size[0]; i++){
                    for (int j = 0; j < rsThImage.size[1]; j++){
                        cv::Vec3b pixel = rsThImage.at<cv::Vec3b>(i, j);
                        if (pixel.val[0] > 0 || pixel.val[1] > 0 || pixel.val[2] > 0){
                            pair<double, double> point;
                            point.first = j;
                            point.second = i;
                            dataset.push_back(point);
                            k++;
                        }
                    }
                }

                unsigned int countOutliers = dataset.size();
                if (countOutliers > 0){
                    vector<int> labels = this->dbscan->fit(dataset);
                    int maxLabel = -1;
                    for (unsigned int i = 0; i < labels.size(); i++){
                        if (labels[i] > maxLabel) maxLabel = labels[i];
                    }

                    BB.clear();

                    for (int i = 0; i <= maxLabel; i++){
                        cv::Rect r = cv::Rect(INT_MAX, INT_MAX, 0, 0);
                        BB.push_back(r);
                        for (unsigned int l = 0; l < countOutliers; l++){
                            if (labels[l] == i){
                                if (dataset[l].first < BB[i].x){
                                    BB[i].x = dataset[l].first;
                                }
                                if (dataset[l].second < BB[i].y){
                                    BB[i].y = dataset[l].second;
                                }
                                if (dataset[l].first > BB[i].width){
                                    BB[i].width = dataset[l].first;
                                }
                                if (dataset[l].second > BB[i].height){
                                    BB[i].height = dataset[l].second;
                                }
                            }
                        }
                        BB[i].x      = max((int)(BB[i].x * this->m_invscale * 4 - BB_OFFSET), 0);
                        BB[i].y      = max((int)(BB[i].y * this->m_invscale * 4 - BB_OFFSET), 0);
                        BB[i].width  = min((int)(BB[i].width * this->m_invscale * 4 + BB_OFFSET * 2), image.cols);
                        BB[i].height = min((int)(BB[i].height * this->m_invscale * 4 + BB_OFFSET * 2), image.rows);
                    }
                }

                for (int i = 0; i < BB.size(); i++){
                    if (BB[i].width == BB_OFFSET * 2 && BB[i].height == BB_OFFSET * 2){
                        BB.erase(BB.begin() + i);
                        i--;
                    }
                }

                for (unsigned int i = 0; i < BB.size(); i++){
                    rectangle(image, cv::Point(BB[i].x, BB[i].y), cv::Point(BB[i].width, BB[i].height), cv::Scalar(0, 255, 255), 3, 0);
                }
                cv::Mat outputScreen(FINAL_HEIGHT, FINAL_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
                cv::Mat rsim;
                cv::resize(image, rsim, cv::Size(FINAL_WIDTH - DOWNSAMPLED_WIDTH, FINAL_HEIGHT - DOWNSAMPLED_HEIGHT), 0, 0);
                cv::Mat roi(outputScreen, cv::Rect(0, 0, rsim.cols, rsim.rows));
                rsim.copyTo(roi);
                roi = cv::Mat(outputScreen, cv::Rect(FINAL_WIDTH - DOWNSAMPLED_WIDTH, 0, baseline.cols, baseline.rows));
                cv::Mat baselineTitle = makeTitleBar(TITLE_BASELINE, baseline);

                baselineTitle.copyTo(roi);
                roi = cv::Mat(outputScreen, cv::Rect(FINAL_WIDTH - DOWNSAMPLED_WIDTH, DOWNSAMPLED_HEIGHT, stdImage.cols, stdImage.rows));
                cv::Mat stdInt;
                stdImage.convertTo(stdInt, CV_8UC3);
                cv::Mat stdTitle = makeTitleBar(TITLE_SD, stdInt);
                stdTitle.copyTo(roi);
                roi = cv::Mat(outputScreen, cv::Rect(FINAL_WIDTH - DOWNSAMPLED_WIDTH, DOWNSAMPLED_HEIGHT * 2, outliers.cols, outliers.rows));
                cv::Mat outliersTitle = makeTitleBar(TITLE_SUBTRACTED, outliers);
                outliersTitle.copyTo(roi);
                cv::Mat closed;
                cv::Mat kernel2(10, 10, CV_8U, cv::Scalar(1));
                cv::morphologyEx(thImage, closed, cv::MORPH_DILATE, kernel2);
                cv::morphologyEx(closed, closed, cv::MORPH_CLOSE, kernel);
                closed = rsImage.mul(closed/255.0);
                roi = cv::Mat(outputScreen, cv::Rect(FINAL_WIDTH - DOWNSAMPLED_WIDTH, DOWNSAMPLED_HEIGHT * 4, maskedOutliers.cols, maskedOutliers.rows));
                cv::Mat maskedOutliersTitle = makeTitleBar(TITLE_MASKED, closed);
                maskedOutliersTitle.copyTo(roi);

                roi = cv::Mat(outputScreen, cv::Rect(FINAL_WIDTH - DOWNSAMPLED_WIDTH, DOWNSAMPLED_HEIGHT * 3, thImage.cols, thImage.rows));
                cv::Mat thTitle = makeTitleBar(TITLE_THRESHOLDED, thImage);
                thTitle.copyTo(roi);

                roi = cv::Mat(outputScreen, cv::Rect(0, FINAL_HEIGHT - DOWNSAMPLED_HEIGHT, menu.cols, menu.rows));
                menu.copyTo(roi);

                cv::resize(outputScreen, output, cv::Size(this->m_horizontal, this->m_vertical), 0, 0);
                return BB;
}

cv::Mat ObjectDetector::buildBaseline(vector<cv::Mat>& imageSet, cv::Mat& image){
    int size[4] = { image.size[0], image.size[1], 3, static_cast<int>(imageSet.size()) };
    //int h = image.size[0];
    //int w = image.size[1];
    int* I = (int *)alloca(sizeof(int) * static_cast<unsigned int>(size[0] * size[1]));
    //int I[h][w];
    int* minI = (int *)alloca(sizeof(int) * static_cast<unsigned int>(size[0] * size[1]));

    for (int i = 0; i < size[0]; i++){
        for (int j = 0; j < size[1]; j++){
            minI[i * size[0] + j] = INT_MAX;
        }
    }

    for (int l = 0; l < size[3]; l++){
        for (int i = 0; i < size[0]; i++){
            for (int j = 0; j < size[1]; j++){
                double diff = 0;
                cv::Vec3b p1 = imageSet[l].at<cv::Vec3b>(i, j);
                cv::Vec3b p2 = image.at<cv::Vec3b>(i, j);
                for (int k = 0; k < size[2]; k++){
                    diff += abs(p1.val[k] - p2.val[k]);
                }
                if (diff < minI[i * size[0] + j]){
                    minI[i * size[0] + j] = static_cast<int>(diff);
                    I[i * size[0] + j] = l;
                }
            }
        }
    }

    cv::Mat similar(size[0], size[1], CV_8UC3, cv::Scalar(255));

    for (int i = 0; i < size[0]; i++){
        for (int j = 0; j < size[1]; j++){
            int l = I[i * size[0] + j];
            cv::Vec3b pixel = imageSet[l].at<cv::Vec3b>(i, j);
            similar.at<cv::Vec3b>(i, j) = pixel;
        }
    }
    return similar;
}

