#include <QCoreApplication>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "objectdetector.h"
#include "caninterface.h"

using namespace std;

bool fileExists(const char* fileName){
    ifstream infile(fileName);
    return infile.good();
}

//vector<int> rect2bytevec(cv::Rect r){
//    vector<int> message;
//    vector<int> input = {r.x, r.y, r.width, r.height};
//    for (unsigned int i = 0; i < input.size(); i++){
//        //int n = input[i];
//        int8_t hi = static_cast<int8_t>((input[i] >> 8) & 0xff);
//        int8_t lo = static_cast<int8_t>((input[i] >> 0) & 0xff);
//        message = {hi, lo};
//    }
//    return message;
//}

int main(int argc, char *argv[]){
    int canID = 100;
    bool canEnabled = false;
    cv::VideoWriter outputVideo;
    static bool recordVideo = false;

    cv::VideoCapture stream1;
    cv::Mat cameraFrame, mask;

    cout << stream1.isOpened() << endl;
    if (!stream1.isOpened()) {
        cout << "Camera initialized..." << endl;
        if (argc >= 2){
            string camera = argv[1];
            if (camera == "gopro"){
                //todo
                cout << "To be implemented..." << endl;
                return -1;
            }else{
                int camera_id = stoi(camera);
                stream1 = cv::VideoCapture(camera_id);
                stream1.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
                stream1.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
                stream1.read(cameraFrame);
            }
            if (argc >= 3){
                string maskfile = argv[2];
                if (maskfile != "default"){
                    if (fileExists(maskfile.c_str())){
                        mask = cv::imread(maskfile);
                        if (!(mask.size[0] == cameraFrame.size[0] && mask.size[1] == cameraFrame.size[1])){
                            cout << "Mask file does not exist! Using default mask." << endl;
                            mask = cv::Mat(cameraFrame.size[0], cameraFrame.size[1], CV_8UC3, cv::Scalar(255, 255, 255));
                        }
                    }else{
                        cout << "Mask file does not exist! Using default mask." << endl;
                        mask = cv::Mat(cameraFrame.size[0], cameraFrame.size[1], CV_8UC3, cv::Scalar(255, 255, 255));
                    }
                }
                if (argc >= 4){
                    canID = stoi(argv[3]);
                    canEnabled = true;
                    cout << "CAN writing enabled." << endl;
                }
            }else{
                cout << "Using default mask..." << endl;
                mask = cv::Mat(cameraFrame.size[0], cameraFrame.size[1], CV_8UC3, cv::Scalar(255, 255, 255));
            }
        }else{
            stream1 = cv::VideoCapture(0);
            stream1.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
            stream1.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
            stream1.read(cameraFrame);
            cout << "Using default mask..." << endl;
            mask = cv::Mat(cameraFrame.size[0], cameraFrame.size[1], CV_8UC3, cv::Scalar(255, 255, 255));
        }
    }else{
        cout << "Cannot open camera!" << endl;                  //check if video device has been initialised
        return -1;
    }

    imshow("Anomaly Detection - Demo", cameraFrame);
    cv::moveWindow("Anomaly Detection - Demo", 0, 0);

    //    cv::namedWindow("Anomaly Detection - Demo", cv::WINDOW_NORMAL);
    //    cv::setWindowProperty("Anomaly Detection - Demo", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    int threshold = 10, clusterSize = 33, epsilon = 27;
    int temp1, temp2, temp3;
    temp1 = threshold;
    temp2 = clusterSize;
    temp3 = epsilon;

    cv::createTrackbar("Threshold", "Anomaly Detection - Demo",    &threshold,   100);
    cv::createTrackbar("Cluster Size", "Anomaly Detection - Demo", &clusterSize, 100);
    cv::createTrackbar("Sparsity", "Anomaly Detection - Demo",     &epsilon,     100);

    unsigned int timerOverlay = 0;
    string messageOverlay = "";
    ObjectDetector* od = new ObjectDetector(0.1, 10, mask);

    while (stream1.isOpened()) {
        stream1.read(cameraFrame);

        int key = cv::waitKey(30);

        if (key == 98){                                                                                     //"b", add frame to baseline;
            od->addToBaseline(cameraFrame);
            timerOverlay = 5;
            messageOverlay = "Frame #" + to_string(od->getImageSetSize()) + " added to the baseline.";
        }else if (key == 113){                                                                              //"q", Quit demo
            cv::destroyAllWindows();
            delete od;
            return 0;
        }else if (key == 114){                                                                              //"r", Reset basline;
            od->clearBaseline();
            timerOverlay = 5;
            messageOverlay = "Baseline reset.";
        }else if (key == 116 || threshold > temp1){                                                         //"t", increase threshold;
            threshold = min(threshold + 1, 100);
            od->setThreshold(threshold / 100.0);
            timerOverlay = 5;
            messageOverlay = "Threshold = " + to_string(threshold) + "%.";
            temp1 = threshold;
        }else if (key == 84 || threshold < temp1){                                                          //"T", decrease threshold;
            threshold = max(threshold - 1, 0);
            od->setThreshold(threshold / 100.0);
            timerOverlay = 5;
            messageOverlay = "Threshold = " + to_string(threshold) + "%.";
            temp1 = threshold;
        }else if (key == 99 || clusterSize > temp2){                                                        //"c", increase min cluster size;
            clusterSize = min(clusterSize + 1, 100);
            od->setClusterSize(static_cast<unsigned int>(clusterSize / 10 / 3));
            timerOverlay = 5;
            messageOverlay = "Minimum Cluster Size = " + to_string(clusterSize) + "%.";
            temp2 = clusterSize;
        }else if (key == 67 || clusterSize < temp2){                                                        //"C", decrease min cluster size;
            clusterSize = max(clusterSize - 1, 0);
            od->setClusterSize(static_cast<unsigned int>(clusterSize / 10 / 3));
            timerOverlay = 5;
            messageOverlay = "Minimum Cluster Size = " + to_string(clusterSize) + "%.";
            temp2 = clusterSize;
        }else if (key == 115 || epsilon > temp3){                                                           //"s", increase cluster sparsity;
            epsilon = min(epsilon + 1, 100);
            od->setEpsilon(epsilon / 10.0 / 3.0);
            timerOverlay = 5;
            messageOverlay = "Cluster Sparsity = " + to_string(epsilon) + "%.";
            temp3 = epsilon;
        }else if (key == 83 || epsilon < temp3){                                                            //"S", decrease cluster sparsity;
            epsilon = max(epsilon - 1, 0);
            od->setEpsilon(epsilon / 10.0 / 3.0);
            timerOverlay = 5;
            messageOverlay = "Cluster Sparsity = " + to_string(epsilon) + "%.";
            temp3 = epsilon;
        }

        cv::Mat outputImage;
        vector<cv::Rect> BB = od->evaluate(cameraFrame, outputImage);

        //        CanInterface* canInterface = nullptr;
        //        if (canEnabled){
        //            canInterface = new CanInterface(1, "500K");
        //            canInterface->connect();
        //            vector<int> anomaly = {static_cast<int>(BB.size())};
        //            canInterface->write(canID, anomaly);
        //            for (unsigned int i = 0; i < BB.size(); i++){
        //                canInterface->write(canID + i + 1, rect2bytevec(BB[i]));
        //            }
        //        }

        if (timerOverlay != 0){
            putText(outputImage, messageOverlay, cv::Point2f(100, 100), cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 255, 255, 255), 2);
            timerOverlay--;
        }

        if (key == 118){    //Record Video
            recordVideo = !recordVideo;
            cout << "Record video = " << recordVideo << endl;
            if (recordVideo){
                outputVideo.open("video.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 20.0, cv::Size(outputImage.cols, outputImage.rows), true);
            }
        }

        if (recordVideo){
            outputVideo << outputImage;
        }
        imshow("Anomaly Detection - Demo", outputImage);
    }
    return 0;
}
