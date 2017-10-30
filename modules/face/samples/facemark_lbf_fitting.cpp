/*
This file was part of GSoC Project: Facemark API for OpenCV
Final report: https://gist.github.com/kurnianggoro/74de9121e122ad0bd825176751d47ecc
Student: Laksono Kurnianggoro
Mentor: Delia Passalacqua
*/

/*----------------------------------------------
 * Usage:
 * facemark_lbf_fitting <face_cascade_model> <lbf_model> <video_name>
 *
 * example:
 * facemark_lbf_fitting ../face_cascade.xml ../LBF.model ../video.mp4
 *
 * note: do not forget to provide the LBF_MODEL and DETECTOR_MODEL
 * the model are available at opencv_contrib/modules/face/data/
 *--------------------------------------------------*/

#include <stdio.h>
#include <ctime>
#include <iostream>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/face.hpp"

using namespace std;
using namespace cv;
using namespace cv::face;

CascadeClassifier face_cascade;
bool myDetector( InputArray image, OutputArray ROIs, void * config = 0);
bool parseArguments(int argc, char** argv, CommandLineParser & parser,
    String & cascade, String & model,String & video);

int main(int argc, char** argv ){
    CommandLineParser parser(argc, argv,"");
    String cascade_path,model_path,images_path, video_path;
    if(!parseArguments(argc, argv, parser,cascade_path,model_path,video_path))
       return -1;

    face_cascade.load(cascade_path);

    FacemarkLBF::Params params;
    params.model_filename = model_path;
    params.cascade_face = cascade_path;

    Ptr<Facemark> facemark = FacemarkLBF::create(params);
    facemark->setFaceDetector(myDetector);
    facemark->loadModel(params.model_filename.c_str());

    VideoCapture capture(video_path);
    Mat frame;

    if( !capture.isOpened() ){
        printf("Error when reading vide\n");
        return 0;
    }

    Mat img;
    String text;
    char buff[255];
    double fittime;
    int nfaces;
    std::vector<Rect> rects,rects_scaled;
    std::vector<std::vector<Point2f> > landmarks;
    CascadeClassifier cc(params.cascade_face.c_str());
    namedWindow( "w", 1);
    for( ; ; )
    {
        capture >> frame;
        if(frame.empty())
            break;

        double __time__ = (double)getTickCount();

        float scale = (float)(400.0/frame.cols);
        resize(frame, img, Size((int)(frame.cols*scale), (int)(frame.rows*scale)));

        facemark->getFaces(img, rects);
        rects_scaled.clear();

        for(int j=0;j<(int)rects.size();j++){
            rects_scaled.push_back(Rect(
                (int)(rects[j].x/scale),
                (int)(rects[j].y/scale),
                (int)(rects[j].width/scale),
                (int)(rects[j].height/scale)));
        }
        rects = rects_scaled;
        fittime=0;
        nfaces = (int)rects.size();
        if(rects.size()>0){
            double newtime = (double)getTickCount();

            facemark->fit(frame, rects, landmarks);


            fittime = ((getTickCount() - newtime)/getTickFrequency());
            for(int j=0;j<(int)rects.size();j++){
                landmarks[j] = Mat(Mat(landmarks[j]));
                drawFacemarks(frame, landmarks[j], Scalar(0,0,255));
            }
        }


        double fps = (getTickFrequency()/(getTickCount() - __time__));
        sprintf(buff, "faces: %i %03.2f fps, fit:%03.0f ms",nfaces,fps,fittime*1000);
        text = buff;
        putText(frame, text, Point(20,40), FONT_HERSHEY_PLAIN , 2.0,Scalar::all(255), 2, 8);

        imshow("w", frame);
        waitKey(1); // waits to display frame
    }
    waitKey(0); // key press to close window
}

bool myDetector( InputArray image, OutputArray ROIs, void * config ){
    Mat gray;
    std::vector<Rect> & faces = *(std::vector<Rect>*) ROIs.getObj();
    faces.clear();

    if(config!=0){
        //do nothing
    }

    if(image.channels()>1){
        cvtColor(image.getMat(),gray,CV_BGR2GRAY);
    }else{
        gray = image.getMat().clone();
    }
    equalizeHist( gray, gray );

    face_cascade.detectMultiScale( gray, faces, 1.4, 2, CV_HAAR_SCALE_IMAGE, Size(30, 30) );
    return true;
}

bool parseArguments(int argc, char** argv, CommandLineParser & parser,
    String & cascade,
    String & model,
    String & video
){
    const String keys =
        "{ @c cascade         |      | (required) path to the cascade model file for the face detector }"
        "{ @m model           |      | (required) path to the trained model }"
        "{ @v video           |      | (required) path input video}"
        "{ help h usage ?     |      | facemark_lbf_fitting -cascade -model -video [-t]\n"
             " example: facemark_lbf_fitting ../face_cascade.xml ../LBF.model ../video.mp4}"
    ;
    parser = CommandLineParser(argc, argv,keys);
    parser.about("hello");

    if (parser.has("help")){
        parser.printMessage();
        return false;
    }

    cascade = String(parser.get<String>("cascade"));
    model = String(parser.get<string>("model"));
    video = String(parser.get<string>("video"));


    if(cascade.empty() || model.empty() || video.empty() ){
        std::cerr << "one or more required arguments are not found" << '\n';
        cout<<"cascade : "<<cascade.c_str()<<endl;
        cout<<"model : "<<model.c_str()<<endl;
        cout<<"video : "<<video.c_str()<<endl;
        parser.printMessage();
        return false;
    }

    return true;
}
