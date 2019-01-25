g++ \
-o test -std=gnu++11 \
-Wno-deprecated-declarations -Wno-unused-result \
main.cpp \
Dk/ManagerConnection.cpp Dk/IpAdress.cpp Dk/Protocole.cpp Dk/Server.cpp Dk/Socket.cpp Dk/VideoStreamWriter.cpp \
Tool/MovieWriter.cpp Tool/Recorder.cpp \
Device/Device.cpp Device/DeviceMT.cpp \
-lpthread \
-lx264 -lswscale -lavutil -lavformat -lavcodec

# -I/usr/local/include/opencv4/ 
# -I/usr/local/lib/libjpeg-turbo-1.5.1/
# -lturbojpeg -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs
