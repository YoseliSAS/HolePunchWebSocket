cmake_minimum_required(VERSION 3.10)

project(WebSocketGStreamer)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0)
pkg_check_modules(SOUP REQUIRED libsoup-3.0)
pkg_search_module(JSONC REQUIRED json-c)

include_directories(${GST_INCLUDE_DIRS} ${SOUP_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS} ${JSONC_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS} ${SOUP_LIBRARY_DIRS})

add_executable(WebSocketGStreamer main.cpp)

target_link_libraries(WebSocketGStreamer ${GST_LIBRARIES} ${SOUP_LIBRARIES} ${JSONC_LIBRARIES})
