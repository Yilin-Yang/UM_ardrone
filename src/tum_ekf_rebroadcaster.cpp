#include "um_ardrone/tum_ekf_rebroadcaster.h"
  using boost::make_shared;
  using std::array;
  using std::string;

  using nav_msgs::Odometry;
  using geometry_msgs::Pose;
  using geometry_msgs::PoseWithCovariance;
  using geometry_msgs::Twist;
  using geometry_msgs::TwistWithCovariance;
  using tum_ardrone::filter_state;

#include "um_ardrone/math.h"

#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>

#include <cstring>
  using std::memcpy;

namespace um_ardrone
{

TumEkfRebroadcaster::TumEkfRebroadcaster(
  const string& subscribed_topic,
  const string& published_topic,
  const string& world_tf_frame_id_in,
  const string& local_tf_frame_id_in,
  const array<double, 36>& pose_covar_in,
  const array<double, 36>& twist_covar_in,
  size_t max_sub_queue_size,
  size_t max_pub_queue_size
)
: TemplatedRebroadcaster(
    subscribed_topic,
    published_topic,
    max_sub_queue_size,
    max_pub_queue_size
  ),
  world_tf_frame_id{world_tf_frame_id_in},
  local_tf_frame_id{local_tf_frame_id_in},
  pose_covar{pose_covar_in},
  twist_covar{twist_covar_in}
{
  ROS_INFO("TumEkfRebroadcaster world_tf_frame: %s", world_tf_frame_id.c_str());
  ROS_INFO("TumEkfRebroadcaster local_tf_frame: %s", local_tf_frame_id.c_str());
}

void TumEkfRebroadcaster::setPose(
  const filter_state::ConstPtr& from_ptr,
  Odometry::Ptr& in
)
{
  const filter_state& from = *from_ptr;

  PoseWithCovariance& pose_with_cov = in->pose;
  Pose& pose = pose_with_cov.pose;

  memcpy(pose_with_cov.covariance.data(), pose_covar.data(), NUM_MATRIX_CHARS);

  pose.position.x = from.x;
  pose.position.y = from.y;
  pose.position.z = from.z;

  {
    tf2::Quaternion orientation;
    orientation.setRPY(
      degToRad(from.roll),
      degToRad(from.pitch),
      degToRad(from.yaw)
    );

    pose.orientation = tf2::toMsg(orientation);
  }
}

void TumEkfRebroadcaster::setTwist(
  const filter_state::ConstPtr& from_ptr,
  Odometry::Ptr& in
)
{
  const filter_state& from = *from_ptr;

  TwistWithCovariance& twist_with_cov = in->twist;
  Twist& twist = twist_with_cov.twist;

  memcpy(twist_with_cov.covariance.data(), twist_covar.data(), NUM_MATRIX_CHARS);

  twist.linear.x = from.dx;
  twist.linear.y = from.dy;
  twist.linear.z = from.dz;

  twist.angular.x = 0;
  twist.angular.y = 0;
  twist.angular.z = degToRad(from.dyaw);
}

Odometry::ConstPtr TumEkfRebroadcaster::convertSubToPub(
  const filter_state::ConstPtr& received_ptr
)
{
  Odometry::Ptr estimate_msg =
    make_shared<Odometry>();

  TumEkfRebroadcaster::setPose (received_ptr, estimate_msg);
  TumEkfRebroadcaster::setTwist(received_ptr, estimate_msg);

  estimate_msg->header = received_ptr->header;

  estimate_msg->header.frame_id = world_tf_frame_id;
  estimate_msg->child_frame_id  = local_tf_frame_id;

  return estimate_msg;
}

} // namespace um_ardrone
