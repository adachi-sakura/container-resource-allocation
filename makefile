server: main.cpp Genetic.cpp GeneticSerializableObjects.cpp thirdparty/jsonxx/jsonxx/jsonxx.cc
	g++ -Ithirdparty/crow -Ithirdparty/jsonxx -std=c++11 $^ -lpthread -lboost_system -lboost_filesystem -o $@
gamsp: main.cpp GAMSP.cpp GAMSP_web.cpp thirdparty/jsonxx/jsonxx/jsonxx.cc
	g++ -Ithirdparty/crow -Ithirdparty/jsonxx -std=c++11 $^ -lpthread -lboost_system -lboost_filesystem -o $@