cmake_minimum_required(VERSION 3.19)
project(ff_player LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()


set(TARGET_NAME ff_player)
qt_add_executable(${TARGET_NAME}
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    ff_class.h 
    parser.h 
    
    
    
    page_play.h
    page_log.h
    ring_buffer.h
    player.h
    
)

target_link_libraries(ff_player
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS ff_player
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ff_player
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# FFmpeg
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/ffmpeg_official)
target_include_directories(${TARGET_NAME} PRIVATE ${FFMPEG_DIR}/include)
find_library(FF_AVFORMAT_LIB avformat ${FFMPEG_DIR}/lib)
find_library(FF_AVCODEC_LIB avcodec ${FFMPEG_DIR}/lib)
find_library(FF_AVUTIL_LIB avutil ${FFMPEG_DIR}/lib)
find_library(FF_SWSCALE_LIB swscale ${FFMPEG_DIR}/lib)
target_link_libraries(${TARGET_NAME} PRIVATE
    ${FF_AVFORMAT_LIB}
    ${FF_AVCODEC_LIB}
    ${FF_AVUTIL_LIB}
    ${FF_SWSCALE_LIB}
)

# json
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)