server: main.cpp jsonxx/jsonxx.cc
        g++ -Iinclude -lpthread -lboost_system-mt -lboost_filesystem -std=c++11 main.cpp jsonxx/jsonxx.cc -o server