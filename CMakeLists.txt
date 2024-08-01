cmake_minimum_required(VERSION 3.10)

project(FFmpeg_Demo LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/ffmpegInclude)

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} 
	${CMAKE_SOURCE_DIR}/ffmpegLib/libavcodecd.lib
	${CMAKE_SOURCE_DIR}/ffmpegLib/libavdeviced.lib
	${CMAKE_SOURCE_DIR}/ffmpegLib/libavfilterd.lib
	${CMAKE_SOURCE_DIR}/ffmpegLib/libavformatd.lib
	${CMAKE_SOURCE_DIR}/ffmpegLib/libavutild.lib
	${CMAKE_SOURCE_DIR}/ffmpegLib/libpostprocd.lib
	${CMAKE_SOURCE_DIR}/ffmpegLib/libswresampled.lib
	${CMAKE_SOURCE_DIR}/ffmpegLib/libswscaled.lib
)