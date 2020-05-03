//#include "include/crow.h"
#include "Genetic.h"
#include "test.h"
//#include "include/jsonxx.h"
using namespace std;
//using namespace jsonxx;

int main() {
    //test();
    auto params = getDefaultParameters();
    testRoundRobin(params);
    testRandom(params);
    testKubernetes(params);
    testGenetic(params);
    return 0;
//    crow::SimpleApp app;
//    CROW_ROUTE(app, "/ping").methods("GET"_method)([](){
//        return "pong";
//    });
//    CROW_ROUTE(app, "/algorithm").methods("GET"_method)([](const crow::request & req){
//        string json = req.body;
//        AlgorithmParameters params(json);
//        string err;
//        auto res = run(params, err);
//        if (res.size() == 0)
//        {
//            return crow::response(400, err);
//        }
//        Array ret;
//        for(auto & microservice : res)
//        {
//            ret << microservice.object();
//        }
//        return crow::response(200, ret.json());
//    });
//    app.port(8080).multithreaded().run();
//    return 0;
}