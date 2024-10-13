cmake_minimum_required(VERSION 3.8)
project(mqtt_client)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++14)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
     
file(GLOB_RECURSE srcs "include/*.cpp")
file(GLOB_RECURSE header_file "include/*.h" "include/*.hpp")

install(FILES ${header_file} DESTINATION  include/${PROJECT_NAME})

ament_export_include_directories(include)

ament_package()