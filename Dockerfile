FROM adachisakura/ubuntu-boost:latest
COPY . container-allocation/
RUN apt-get -qq update && apt-get -qqy install make
RUN cd container-allocation && g++ -Ithirdparty/crow -Ithirdparty/jsonxx -std=c++11 main.cpp Genetic.cpp GeneticSerializableObjects.cpp thirdparty/jsonxx/jsonxx/jsonxx.cc -lpthread -lboost_system -lboost_filesystem -o server
EXPOSE 8080
CMD ["container-allocation/server"]
