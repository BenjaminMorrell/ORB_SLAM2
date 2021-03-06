/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/


#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>

#include<ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <std_msgs/String.h>
#include <cv_bridge/cv_bridge.h>

#include<opencv2/core/core.hpp>

#include"../../../include/System.h"
#include "../../../include/Converter.h"

using namespace std;

class ImageGrabber
{
public:
    ImageGrabber(ORB_SLAM2::System* pSLAM):mpSLAM(pSLAM){}

    void GrabImage(const sensor_msgs::ImageConstPtr& msg);

    ORB_SLAM2::System* mpSLAM;

    // setting up transform 
    cv::Mat currentPosFrame;
    tf::Transform transform_current;
    tf::Quaternion q;
    tf::TransformBroadcaster br;
    vector<float> q_temp;
    cv::Mat R;
};

int main(int argc, char **argv)
{
    ros::init(argc, argv, "Mono");
    ros::start();

    if(argc != 3)
    {
        cerr << endl << "Usage: rosrun ORB_SLAM2 Mono path_to_vocabulary path_to_settings" << endl;        
        ros::shutdown();
        return 1;
    }    

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::MONOCULAR,true);

    ImageGrabber igb(&SLAM);

    ros::NodeHandle nodeHandler;
    ros::Subscriber sub = nodeHandler.subscribe("/camera/image_raw", 1, &ImageGrabber::GrabImage,&igb);

    ros::spin();

    // Save camera trajectory
    SLAM.SaveKeyFrameTrajectoryTUM("/home/bjm/postdoc/Results/KeyFrameTrajectory.txt");
    // SLAM.SaveTrajectoryTUM("/home/bjm/postdoc/data/FrameTrajectory_TUM_Format.txt");
    // SLAM.SaveTrajectoryKITTI("/home/bjm/postdoc/data/FrameTrajectory_KITTI_Format.txt");

    // Save Map points
    SLAM.SaveMapPoints("/home/bjm/postdoc/Results/ORBSLAMMapPoints.txt");
    
    // Stop all threads
    SLAM.Shutdown();
    
    ros::shutdown();

    return 0;
}

void ImageGrabber::GrabImage(const sensor_msgs::ImageConstPtr& msg)
{
    // Copy the ros image message to cv::Mat.
    cv_bridge::CvImageConstPtr cv_ptr;
    try
    {
        cv_ptr = cv_bridge::toCvShare(msg);
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
    }

    mpSLAM->TrackMonocular(cv_ptr->image,cv_ptr->header.stamp.toSec());

    // publish tf 
    currentPosFrame = mpSLAM->GetFrame();
    cout << "matrix out is " << currentPosFrame << endl;
    
    cv::Size s = currentPosFrame.size();

    if (s.width == 4)
    {
        R = currentPosFrame(cv::Range(0,2), cv::Range(0,2));
        q_temp = ORB_SLAM2::Converter::toQuaternion(R);
        tf::Quaternion quat(q_temp[0],q_temp[1],q_temp[2],q_temp[3]);
        tf::Vector3 vec(currentPosFrame.at<float>(0, 3),currentPosFrame.at<float>(1, 3), currentPosFrame.at<float>(2, 3));
        vec.rotate(quat.inverse().getAxis(),quat.inverse().getAngle());
        transform_current.setOrigin(-vec);

        //transform_current.setOrigin(tf::Vector3(-currentPosFrame.at<float>(0, 3),-currentPosFrame.at<float>(1, 3), -currentPosFrame.at<float>(2, 3)));
        transform_current.setRotation(quat.inverse());
    
       // transform_current.setRotation(tf::Quaternion(q_temp[0],q_temp[1],q_temp[2],q_temp[3]).inverse());
        br.sendTransform(tf::StampedTransform(transform_current, ros::Time::now(), "world","camera"));
        
    }
       else {
        cout << "tf matrix is empty " << endl;
    }
}


