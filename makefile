server: main.cpp Genetic.cpp jsonxx/jsonxx.cc
    g++ -Iinclude -std=c++11 main.cpp Genetic.cpp jsonxx/jsonxx.cc -lpthread -lboost_system-mt -lboost_filesystem -o server