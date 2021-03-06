#include "faceimageprocess.h"

FaceImageProcess::FaceImageProcess(QObject *parent) : QObject(parent)
{
    faceDetect = vision::instantiateVisionFaceDetect();
    faceAlignment = vision::instantiateVisionFaceAlignment();

    registerFlag = false;
    recogniseCoreAvaliable = true;
    useBestFace = false;
    currIdx = 0;

    // save the recognise history to ./history
    historyDir = "./history/";
    QDir *dir = new QDir();
    if(!dir->exists(historyDir))
    {
        dir->mkdir(historyDir);
    }

    //	 model_points.push_back(cv::Point3d((-5.311432f - 1.789930f) / 2, (5.485328f + 5.393625f) / 2, (3.987654f + 4.413414f) / 2));//left eye middle
    //	 model_points.push_back(cv::Point3d( (5.311432f + 1.789930f) / 2, (5.485328f + 5.393625f) / 2, (3.987654f + 4.413414f) / 2));//right eye middle
    //	 model_points.push_back(cv::Point3d( 0.000000f, 0.000000f, 6.763430f));//nose tip
    //	 model_points.push_back(cv::Point3d(-2.774015f,-2.080775f, 5.048531f));//mouth left corner
    //	 model_points.push_back(cv::Point3d( 2.774015f,-2.080775f, 5.048531f));//mouth right corner
    model_points.push_back(cv::Point3d(-225.0f, 170.0f, -135.0f));//left eye middle
    model_points.push_back(cv::Point3d(225.0f, 170.0f, -135.0f));//right eye middle
    model_points.push_back(cv::Point3d(0.0f, 0.0f, 0.0f));//nose tip
    model_points.push_back(cv::Point3d(-150.0f, -150.0f, -125.0f));//mouth left corner
    model_points.push_back(cv::Point3d(150.0f, -150.0f, -125.0f));//mouth right corner
}

bool FaceImageProcess::Init()
{
    bool state = faceDetect->Init() && faceAlignment->Init("models/face_align_gray.json");
    if(state)
    {
        emit WriteLogSig("[FaceImageProcess::Init] FaceDetect init succeed.");
        emit WriteLogSig("[FaceImageProcess::Init] FaceKeyPoints init succeed.");
    }
    else
    {
        emit WriteLogSig("[FaceImageProcess::Init] FaceDetect or FaceKeyPoints init FAILED!");
    }
    return state;
}

void FaceImageProcess::SetRecogniseCoreAvaliable()
{
    recogniseCoreAvaliable = true;
}

std::vector<float> FaceImageProcess::GetHeadPos(const cv::Mat& img, const std::vector<cv::Point3d>& model_points, const std::vector<cv::Point2f>& face_points)
{
    std::vector<cv::Point2d> image_points;
    image_points.push_back(cv::Point2d((double)face_points[0].x, (double)face_points[0].y));//left eye middle
    image_points.push_back(cv::Point2d((double)face_points[1].x, (double)face_points[1].y));//right eye middle
    image_points.push_back(cv::Point2d((double)face_points[2].x, (double)face_points[2].y));//nose tip
    image_points.push_back(cv::Point2d((double)face_points[3].x, (double)face_points[3].y));//mouth left corner
    image_points.push_back(cv::Point2d((double)face_points[4].x, (double)face_points[4].y));//mouth right corner

    double focal_length = img.cols; // Approximate focal length.
    cv::Point2d center = cv::Point2d(img.cols/2,img.rows/2);
    cv::Mat camera_matrix = (cv::Mat_<double>(3,3) << focal_length, 0, center.x, 0 , focal_length, center.y, 0, 0, 1);
    cv::Mat dist_coeffs = cv::Mat::zeros(4,1,cv::DataType<double>::type); // Assuming no lens distortion

//    double fx = 500 * double(img.cols) / 640.0;
//    double fy = 500 * double(img.rows) / 480.0;
//    fx = (fx + fy) / 2.0;
//    fy = fx;
//    double cx = double(img.cols) / 2.0;
//    double cy = double(img.rows) / 2.0;
//    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
//    cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, cv::DataType<double>::type);
    cv::Mat rotation_vector;
    cv::Mat translation_vector;
    cv::solvePnP(model_points, image_points, camera_matrix, dist_coeffs, rotation_vector, translation_vector);
    cv::Mat r;//turn rotation vector to rotation matrix
    cv::Rodrigues(rotation_vector, r);
    double angle_x = atan2(r.at<double>(2, 1), r.at<double>(2, 2))*180.0 / CV_PI;
    double angel_y = atan2(-r.at<double>(2, 0), sqrt(r.at<double>(2, 1)*r.at<double>(2, 1) + r.at<double>(2, 2)*r.at<double>(2, 2)))*180.0 / CV_PI;
    double angle_z = atan2(r.at<double>(1, 0), r.at<double>(0, 0))*180.0 / CV_PI;
    std::vector<float> result(3, 0.0);
    if (abs(180.0 - abs(angle_x)) > abs(angle_x))
        result[0] = angle_x;
    else if (angle_x > 0)
        result[0] = angle_x - 180.0;
    else
        result[0] = angle_x + 180.0;
    if (abs(180.0 - abs(angel_y)) > abs(angel_y))
        result[1] = angel_y;
    else if (angel_y > 0)
        result[1] = angel_y - 180.0;
    else
        result[1] = angel_y + 180.0;
    if (abs(180.0 - abs(angle_z)) > abs(angle_z))
        result[2] = angle_z;
    else if (angle_z > 0)
        result[2] = angle_z - 180.0;
    else
        result[2] = angle_z + 180.0;

    return result;
}

void FaceImageProcess::SingleImageProcessing(cv::Mat img)
{
    if(!img.empty())
    {
        cv::Mat img_show = img.clone();
        vector<cv::Rect> face_rects = faceDetect->GetFaces(img);
        for (int i = 0; i < face_rects.size(); i++)
        {
            faceDetect->DrawFaceRect(img_show, face_rects.at(i));
            vector<cv::Point2f> key_points = faceAlignment->GetKeyPoints(img, face_rects.at(i));
            for (int j = 0; j < key_points.size(); j++)
            {
                cv::circle(img_show, key_points.at(j), 2, cv::Scalar(255, 0, 0), 2);
            }
            vector<float> head_pos = GetHeadPos(img, model_points, key_points);
            WriteLogSig(QString::number(head_pos[0]) + " "
                    + QString::number(head_pos[1]) + " "
                    + QString::number(head_pos[2]));
        }

        //emit SendCurrRegFace(normFace);
        //emit SendFrameShow(normFace);   // register
        //if(!registerFlag)
        //{
        //    emit SendFrameToRecognise(normFace);
        //    emit SendFaceImageProcessAvaliable();
        //}

        emit SendFrameForMainWindow(img_show);
    }
}

void FaceImageProcess::DataProcessing(cv::Mat img)
{
    if (!img.empty())
    {
        // std::cout << "Called" << endl;
        cv::Mat img_show = img.clone();
        vector<cv::Rect> face_rects = faceDetect->GetFaces(img);
        for (int i = 0; i < face_rects.size(); i++)
        {
            faceDetect->DrawFaceRect(img_show, face_rects.at(i));
            vector<cv::Point2f> key_points = faceAlignment->GetKeyPoints(img, face_rects.at(i));
            for (int j = 0; j < key_points.size(); j++)
            {
                cv::circle(img_show, key_points.at(j), 2, cv::Scalar(255, 0, 0), 2);
            }
            vector<float> head_pos = GetHeadPos(img, model_points, key_points);
            float tmp_score = (head_pos[0] / 90) * (head_pos[0] / 90) + (head_pos[1] / 90) * (head_pos[1] / 90) + (head_pos[2] / 90) * (head_pos[2] / 90);
            WriteLogSig(QString::number(head_pos[0]) + " "
                    + QString::number(head_pos[1]) + " "
                    + QString::number(head_pos[2]));
            cout << tmp_score << endl;
        }

        //emit SendCurrRegFace(normFace);
        //emit SendFrameShow(normFace);   // register
        //if(!registerFlag)
        //{
        //    emit SendFrameToRecognise(normFace);
        //    emit SendFaceImageProcessAvaliable();
        //}

        emit SendFrameForMainWindow(img_show);
    }
}

FaceImageProcess::~FaceImageProcess()
{

}

