FROM adachisakura/ubuntu-boost:latest
COPY . container-allocation/
RUN cd container-allocation && g++ -Iinclude  -std=c++11 main.cpp Genetic.cpp GeneticSerializableObjects.cpp jsonxx/jsonxx.cc -lpthread -lboost_system -lboost_filesystem -o server
EXPOSE 8080
CMD ["container-allocation/server"]
