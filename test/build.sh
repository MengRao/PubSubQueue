g++ -std=c++11 -O3 -o multhread_test multhread_test.cc -pthread
g++ -std=c++11 -O3 -o pub pub.cc -lrt
g++ -std=c++11 -O3 -o sub sub.cc -lrt
