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
//    vector<int> input,message;

//    input = {r.x, r.y, r.width, r.height};
//    for (unsigned int i = 0; i < input.size(); i++){
//        //int n = input[i];
//        int8_t hi = static_cast<int8_t>((input[i] >> 8) & 0xff);
//        int8_t lo = static_cast<int8_t>((input[i] >> 0) & 0xff);
//        message = {hi, lo};
//    }
//    return message;
//}

static unsigned int canID = 100;
static bool canEnabled = false;
static bool recordVideo = false;
static cv::VideoWriter outputVideo;


int main(int argc, char *argv[]){
    (void) argc; (void) argv;
//    cv::VideoCapture cap(0);
//        if ( !cap.isOpened() ){  // if not success, exit program
//            cout << "Cannot open the web cam" << endl;
//            return -1;
//        }

//        while(true){
//            cv::Mat cameraFrame;
//            cap.read(cameraFrame);
//            imshow("cam", cameraFrame);
//            if (cv::waitKey(1) == 27){
//                if( cap.isOpened() )
//                    cap.release();
//            }
//            if(cv::waitKey(1) == 32){
//                cv::VideoCapture cap(0);
//                if(!cap.isOpened()){
//                    cout << "error occured " << endl;
//                }
//                cap.open(0);
//            }
//        }

    int camera_id = 0;
    cv::VideoCapture stream1;
    cv::Mat cameraFrame,mask;
    if (argc >= 2){
        string camera = argv[1];
        if (camera == "gopro"){
            //todo
            cout << "To be implemented..." << endl;
            return -1;
        }else{
            try{
                camera_id = stoi(camera);
                stream1 = cv::VideoCapture(camera_id);
                stream1.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
                stream1.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
                stream1.read(cameraFrame);
            }
            catch (int e){
                cout << "Invalid argument!" << endl;
                return -1;
            }
        }

        if (argc >= 3){
            string maskfile = argv[2];
            if (maskfile != "default"){
                if (fileExists(maskfile.c_str())){
                    mask =cv::imread(maskfile);
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
                try{
                    canID = static_cast<unsigned int>(stoi(argv[3]));
                    canEnabled = true;
                    cout << "CAN writing enabled." << endl;
                }
                catch (int e){
                    cout << "Invalid argument!" << endl;
                    return -1;
                }
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

    bool flag = 0;
    if (!stream1.isOpened()) { //check if video device has been initialised
        cout << "Cannot open camera!" << endl;
        return -1;
    }else{
        cout << "Camera initialized..." << endl;
        flag = 1;
    }

    imshow("Anomaly Detection - Demo", cameraFrame);
    cv::moveWindow("Anomaly Detection - Demo", 0, 0);

    int threshold = 10, clusterSize = 33, epsilon = 27;
    cv::createTrackbar("Threshold", "Anomaly Detection - Demo", &threshold, 100);
    cv::createTrackbar("Cluster Size", "Anomaly Detection - Demo", &clusterSize, 100);
    cv::createTrackbar("Sparsity", "Anomaly Detection - Demo", &epsilon, 100);

//    cv::namedWindow("Anomaly Detection - Demo", cv::WINDOW_NORMAL);
//    cv::setWindowProperty("Anomaly Detection - Demo", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    ObjectDetector* od = new ObjectDetector(0.1, 10, mask);
    int timerOverlay = 0;
    string messageOverlay = "";
    while (flag) {
        cv::Mat cameraFrame;
        stream1.read(cameraFrame);

        int key = cv::waitKey(30);
        if (key == 98){                                                                                     //"b", add frame to baseline;
            od->addToBaseline(cameraFrame);
            timerOverlay = 5;
            messageOverlay = "Frame #" + to_string(od->getImageSetSize()) + " added to the baseline.";
        }else if (key == 113){                                                                              //"q", Quit demo
            cv::destroyWindow("Anomaly Detection - Demo");
            return 0;
        }else if (key == 114){                                                                              //"r", Reset basline;
            od->clearBaseline();
            timerOverlay = 5;
            messageOverlay = "Baseline reset.";
        }else if (key == 116){                                                                              //"t", increase threshold;
            threshold = min(threshold + 1, 100);
            od->setThreshold(threshold / 100.0);
            timerOverlay = 5;
            messageOverlay = "Threshold = " + to_string(threshold) + "%.";
        }else if (key == 84){                                                                               //"T", decrease threshold;
            threshold = max(threshold - 1, 0);
            od->setThreshold(threshold / 100.0);
            timerOverlay = 5;
            messageOverlay = "Threshold = " + to_string(threshold) + "%.";
        }else if (key == 99){                                                                               //"c", increase min cluster size;
            clusterSize = min(clusterSize + 1, 100);
            od->setClusterSize(static_cast<unsigned int>(clusterSize / 10 / 3));
            timerOverlay = 5;
            messageOverlay = "Minimum Cluster Size = " + to_string(clusterSize) + "%.";
        }else if (key == 67){                                                                               //"C", decrease min cluster size;
            clusterSize = max(clusterSize - 1, 0);
            od->setClusterSize(static_cast<unsigned int>(clusterSize / 10 / 3));
            timerOverlay = 5;
            messageOverlay = "Minimum Cluster Size = " + to_string(clusterSize) + "%.";
        }else if (key == 115){                                                                              //"s", increase cluster sparsity;
            epsilon = min(epsilon + 1, 100);
            od->setEpsilon(epsilon / 10.0 / 3.0);
            timerOverlay = 5;
            messageOverlay = "Cluster Sparsity = " + to_string(epsilon) + "%.";
        }else if (key == 83){                                                                               //"S", decrease cluster sparsity;
            epsilon = max(epsilon - 1, 0);
            od->setEpsilon(epsilon / 10.0 / 3.0);
            timerOverlay = 5;
            messageOverlay = "Cluster Sparsity = " + to_string(epsilon) + "%.";
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

        if (timerOverlay > 0){
            putText(outputImage, messageOverlay, cv::Point2f(100, 100), cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 255, 255, 255), 2);
            timerOverlay--;
        }

        if (key == 118){
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




