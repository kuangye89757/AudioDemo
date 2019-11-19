cmake_minimum_required(VERSION 3.4.1)


# 定义全局变量SOURCES 指定要编译的cpp文件
file(GLOB SOURCE src/main/cpp/*.cpp)

add_library(
        wangyiplayer
        SHARED
        ${SOURCE})

find_library(
        log-lib
        log)

# 头文件存放目录 （编译时需要用到，但不会打到目标库中）
include_directories(src/main/cpp/include)

# 设置环境变量 （这样可以找到相应的库文件）
set(my_lib_path ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI})
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -L${my_lib_path}")


# NDK系统目录下so库可以直接导入 如：libandroid.so
# ${NDK目录}/platforms/android-23/arch-arm/usr/lib

#ibavformat/utils.c:5548: error: undefined reference to 'av_bitstream_filter_filter'
#libavformat/codec2.c:74: error: undefined reference to 'avpriv_codec2_mode_bit_rate'
#libavformat/codec2.c:75: error: undefined reference to 'avpriv_codec2_mode_frame_size'
#libavformat/codec2.c:76: error: undefined reference to 'avpriv_codec2_mode_block_align'
#libavformat/spdifdec.c:63: error: undefined reference to 'av_adts_header_parse'

# libavformat.a要放在libavcodec.a之前
target_link_libraries(
        wangyiplayer
        avfilter avformat avcodec avutil swresample swscale
        ${log-lib}
        android
        z
        OpenSLES
)