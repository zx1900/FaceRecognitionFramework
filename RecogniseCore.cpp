#include "recognisecore.h"

RecogniseCore::RecogniseCore(QObject *parent) : QObject(parent)
{
    faceFeature = vision::instantiateVisionFaceFeature();
    flag = true;
    registNum = 0;
}

bool RecogniseCore::Init()
{
    std::cout << "[RecogniseCore] Try to init face feature." << std::endl;
    bool tmpFlag = faceFeature->Init("models/rog_inception.json");

    if(tmpFlag)
    {
        std::cout << "[RecogniseCore] Init succeed." << std::endl;
    }
    else
    {
        std::cout << "[RecogniseCore] Init FAILED!" << std::endl;
    }

    ReloadDatabaseSlot();

    return tmpFlag;
}

void RecogniseCore::ReloadDatabaseSlot()
{
    registNum = 0;
    // 将已有的人脸Feature读取到databaseFeatures中
    // 先获取总人数
    flag = false;
    featureDatabase.clear();
    QDir *dir = new QDir("./data");
    if(dir->exists())
    {
        QFileInfoList list = dir->entryInfoList();
        for(int i=0; i<list.size(); i++)
        {
            QFileInfo info = list.at(i);
            if(info.isDir())
            {
                QString dirName = info.fileName();
                if(dirName != "." && dirName != "..")
                {
                    registNum++;
                }
            }
        }

        cout << "[RecogniseCore::ReloadDatabaseSlot]# Register number: " << registNum << endl;

        featureDatabase.resize(registNum);
        int idx = 1;
        //读取feature
        for(int i=0; i<list.size(); i++)
        {
            QFileInfo info = list.at(i);
            if(info.isDir())
            {
                QString dirName = info.fileName();
                if(dirName != "." && dirName != "..")
                {
                    featureDatabase[idx-1].name = dirName;
                    featureDatabase[idx-1].imgPath = "./data/" + dirName + "/1.jpg";
                    for(int ii=1; ii<=6; ii++)
                    {
                        QFile file("./data/" + dirName + "/" + QString::number(ii) + ".data");
                        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
                        {
                            QString featureTxt = file.readLine().trimmed();
                            QStringList featureList = featureTxt.split(" ");
                            if(featureList.size() == FEA_LEN)
                            {
                                for(int iii=0; iii<FEA_LEN; iii++)
                                {
                                    switch (ii) {
                                    case 1:
                                        featureDatabase[idx-1].fea1[iii] = featureList.at(iii).toFloat();
                                        break;
                                    case 2:
                                        featureDatabase[idx-1].fea2[iii] = featureList.at(iii).toFloat();
                                        break;
                                    case 3:
                                        featureDatabase[idx-1].fea3[iii] = featureList.at(iii).toFloat();
                                        break;
                                    case 4:
                                        featureDatabase[idx-1].fea4[iii] = featureList.at(iii).toFloat();
                                        break;
                                    case 5:
                                        featureDatabase[idx-1].fea5[iii] = featureList.at(iii).toFloat();
                                        break;
                                    case 6:
                                        featureDatabase[idx-1].fea6[iii] = featureList.at(iii).toFloat();
                                        break;
                                    default:
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    idx++;
                }
            }

            if(i%500 == 0)
            {
                cout << i << " . ";
                cvWaitKey(1);
            }
        }
        cout << endl;
    }
    flag = true;
}

void RecogniseCore::timerEvent(QTimerEvent *)
{
    if(flag)
    {
        flag = false;
    }
    else
    {
        flag = true;
    }
}

void RecogniseCore::RecogniseSlot(cv::Mat img)
{
    if(flag){
        rog_img.release();
        rog_img = img.clone();
        flag = false;

        vector<float> feature = faceFeature->GetFeature(img);

        img.release();
        CompareScore(feature);
        //emit SendAvaliable(true);
        flag = true;
    }
}

void RecogniseCore::SetThd(float i_thd)
{
    thd = i_thd;
}

void RecogniseCore::CompareScore(vector<float> feature)
{
    QVector<QString> resultList;
    resultList.resize(registNum);

    vector<float> score;
    vector<float> scoreTmp;
    score.resize(registNum);
    scoreTmp.resize(registNum);

    for(int i=0; i<featureDatabase.size(); i++)
    {
//        float tmpScore[6];
//        vector<float> fea0;
//        fea0.resize(FEA_LEN);
//        memcpy(fea0.data(), featureDatabase.at(i).fea1, sizeof(float) * FEA_LEN);

//        vector<float> fea1;
//        fea1.resize(FEA_LEN);
//        memcpy(fea1.data(), featureDatabase.at(i).fea2, sizeof(float) * FEA_LEN);

//        vector<float> fea2;
//        fea2.resize(FEA_LEN);
//        memcpy(fea2.data(), featureDatabase.at(i).fea3, sizeof(float) * FEA_LEN);

//        vector<float> fea3;
//        fea3.resize(FEA_LEN);
//        memcpy(fea3.data(), featureDatabase.at(i).fea4, sizeof(float) * FEA_LEN);

//        vector<float> fea4;
//        fea4.resize(FEA_LEN);
//        memcpy(fea4.data(), featureDatabase.at(i).fea5, sizeof(float) * FEA_LEN);

//        vector<float> fea5;
//        fea5.resize(FEA_LEN);
//        memcpy(fea5.data(), featureDatabase.at(i).fea6, sizeof(float) * FEA_LEN);

//        tmpScore[0] = computeScore->GetScore(feature, fea0);
//        tmpScore[1] = computeScore->GetScore(feature, fea1);
//        tmpScore[2] = computeScore->GetScore(feature, fea2);
//        tmpScore[3] = computeScore->GetScore(feature, fea3);
//        tmpScore[4] = computeScore->GetScore(feature, fea4);
//        tmpScore[5] = computeScore->GetScore(feature, fea5);

//        score[i] = (tmpScore[0] + tmpScore[1] + tmpScore[2] + tmpScore[3] + tmpScore[4] + tmpScore[5]) / 6;
//        scoreTmp[i] = score[i];
    }
    vector<int> idx;
    idx.resize(registNum);
    float tmpValue = 100;
    float minValue = 0;
    for(int i=0; i<registNum; i++)
    {
        idx[i] = 0;
        tmpValue = 100;
        for(int j=0; j<registNum; j++)
        {
            if(scoreTmp.at(j) < tmpValue && scoreTmp.at(j) > minValue)
            {
                idx[i] = j;
                tmpValue = scoreTmp.at(j);
            }
        }
        minValue = scoreTmp.at(idx.at(i));
    }

    //    if(score.size() > 0)
    //    {
    //        cout << "people:" << score.size() << " -- " << score.at(idx.at(0)) << "thd:" << thd << endl;
    //    }
    QString info = "";
    bool find = false;
    for(int i=0; i<registNum; i++)
    {
        if(score.at(idx.at(i)) < thd)
        {
            find = true;
            int _idx = idx.at(i);
            cv::Mat img = rog_img.clone();
            info += featureDatabase.at(_idx).name + ":" + QString::number(score[_idx]) + " ";
            emit SendResult(img, featureDatabase.at(_idx).imgPath, featureDatabase.at(_idx).name, score[_idx], gender);
            emit SendResultLog(QDateTime::currentDateTime().toString("hh:mm:ss") + " " + "warnning:" + " a face like " + featureDatabase.at(_idx).name + " was detected.");
        }
        else
        {
            break;
        }
    }

//    if(!find)
//    {
//        Mat img = rog_img.clone();
//        emit SendResult(img, "unknown.jpg", "unknown", score[idx.at(0)], gender);
//    }
    if(info != "")
    {
        emit SendLog("[RecogniseCore::CompareScore] Compare result: " + info);
    }
}


void RecogniseCore::SetAvaliable(bool flag)
{}

RecogniseCore::~RecogniseCore()
{

}
