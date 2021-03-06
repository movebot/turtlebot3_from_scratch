/// \file
/// \brief Publishes coordinates of available landmarks straight from gazebo (no noise) relative to robot
///
/// PARAMETERS:
///   callback_flag (bool): specifies whether to draw landmarks from based on callback trigger
///   map (nuslam::TurtleMap): stores lists of x,y coordinates and radii of landmarks to publish
///   frequency (double): frequency of control loop.
///   frame_id_ (string): frame with respect to which landmark coordinates are published ("base_scan" here)
///
/// PUBLISHES:
///   landmarks (nuslam::TurtleMap): publishes TurtleMap message containing landmark coordinates (x,y) and radii
///
/// SUBSCRIBES:
///   /gazebo/model_states (gazebo_msgs::ModelStates) to read robot and landmark Poses
///
/// FUNCTIONS:
///   gazebo_callback (void): callback for /gazebo/model_states subscriber to get landmark coordinates relative to robot


#include <ros/ros.h>
#include <std_srvs/Empty.h>

#include <math.h>
#include <string>
#include <vector>
#include <boost/iterator/zip_iterator.hpp>
#include "nuslam/TurtleMap.h"
#include <gazebo_msgs/ModelStates.h>
#include <geometry_msgs/Pose.h>

#include <functional>  // To use std::bind
#include <algorithm>  // to use std::find_if
#include "nuslam/landmarks.hpp"

#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>


// Global Vars
bool callback_flag = false;
nuslam::TurtleMap map;
std::string landmark_name = "cylinder";
std::string robot_name = "diff_drive";


void gazebo_callback(const gazebo_msgs::ModelStates &model)
{
  /// \brief extract turtlebot pose and landmark poses from ModelStates,
  /// then computes relative cartesian coordinates to publish as TurtleMap
  /// message with arbitrary radius
  /// \param gazebo_msgs::ModelStates, containing the pose of all
  /// models in the environment

  // First, find current Diff Drive Robot Pose
  auto dd_it = std::find(model.name.begin(), model.name.end(), robot_name);
  auto dd_index = std::distance(model.name.begin(), dd_it);
  geometry_msgs::Pose dd_pose = model.pose.at(dd_index);

  // Used to populate TurtleMap msg
  std::vector<double> radii;
  std::vector<double> x_pts;
  std::vector<double> y_pts;

  auto it = model.name.begin();
  // Find all instances of "cylinder" as the first 8 characters and push back into list of landmarks for TurtleMap
  while ((it = std::find_if(it, model.name.end(), [](std::string model_name){return !model_name.find(landmark_name); })) < model.name.end())
  {
      // Find pose of landmark relative to Diff Drive Robot and store in map
      // std::cout << "NAME: " << *it << std::endl;
      auto index = std::distance(model.name.begin(), it);
      double x_pos = model.pose.at(index).position.x - dd_pose.position.x;
      double y_pos = model.pose.at(index).position.y - dd_pose.position.y;
      rigid2d::Vector2D lnd_pose = rigid2d::Vector2D(x_pos, y_pos);
      nuslam::Point lnd_pt = nuslam::Point(lnd_pose);
      // Adjust r,b for rotation
      auto roll = 0.0, pitch = 0.0, yaw = 0.0;
      tf2::Quaternion quat(dd_pose.orientation.x,\
                           dd_pose.orientation.y,\
                           dd_pose.orientation.z,\
                           dd_pose.orientation.w);
      tf2::Matrix3x3 mat(quat);
      mat.getRPY(roll, pitch, yaw);
      lnd_pt.range_bear.bearing -= yaw;
      nuslam::Point adjusted_landmark = nuslam::Point(lnd_pt.range_bear);
      double radius = 0.12;

      // Populate Vectors
      radii.push_back(radius);
      x_pts.push_back(adjusted_landmark.pose.x);
      y_pts.push_back(adjusted_landmark.pose.y);

      it++;
  }

  // Popoulate TurtleMap msg
  map.radii.clear();
  map.radii = radii;
  map.x_pts.clear();
  map.x_pts = x_pts;
  map.y_pts.clear();
  map.y_pts = y_pts;

  callback_flag = true;
}


int main(int argc, char** argv)
/// The Main Function ///
{
  ROS_INFO("STARTING NODE: analysis");

  double frequency = 60.0;
  std::string frame_id_ = "base_scan";

  ros::init(argc, argv, "analysis"); // register the node on ROS
  ros::NodeHandle nh; // get a handle to ROS
  ros::NodeHandle nh_("~"); // get a handle to ROS
  // Parameters
  nh_.getParam("frequency", frequency);
  nh_.getParam("landmark_frame_id", frame_id_);

  // Publish TurtleMap data wrt this frame
  map.header.frame_id = frame_id_;

  // Init Publishers
  ros::Publisher landmark_pub = nh_.advertise<nuslam::TurtleMap>("landmarks", 1);

  // Init ModelState Subscriber
  ros::Subscriber gzb_sub = nh.subscribe("/gazebo/model_states", 1, gazebo_callback);

  ros::Rate rate(frequency);

  // Main While
  while (ros::ok())
  {
    ros::spinOnce();

    if (callback_flag)
    {
      map.header.stamp = ros::Time::now();
      landmark_pub.publish(map);
      callback_flag = false;
    }

    rate.sleep();
  }

  return 0;
}