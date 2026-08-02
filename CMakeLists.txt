cmake_minimum_required(VERSION 3.10)

project(lightingtest)
add_executable(${PROJECT_NAME} "main.cpp")

target_compile_options(${PROJECT_NAME} PRIVATE
	"-std=c++1z"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	"sfml-system"
	"sfml-window"
	"sfml-graphics"
)
