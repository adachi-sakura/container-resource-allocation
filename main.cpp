#include "thirdparty/crow/crow.h"
//#include "Genetic.h"
//#include "test.h"
//#include "thirdparty/jsonxx/jsonxx.h"
#include "GAMSP_web.h"
#include <vector>
using namespace std;
using namespace jsonxx;


int main() {

//		string json = "{\n"
//		              "    \"nodes\": [\n"
//		              "        {\n"
//		              "            \"name\": \"worker1\",\n"
//		              "            \"milliCore\": 2000,\n"
//		              "            \"mem\": 2000,\n"
//		              "            \"basePrice\": 300,\n"
//		              "            \"unitPrice\": 27.27\n"
//		              "        },\n"
//		              "        {\n"
//		              "            \"name\": \"worker2\",\n"
//		              "            \"milliCore\": 2000,\n"
//		              "            \"mem\": 1800,\n"
//		              "            \"basePrice\": 240,\n"
//		              "            \"unitPrice\": 21.82\n"
//		              "        },\n"
//		              "        {\n"
//		              "            \"name\": \"worker3\",\n"
//		              "            \"milliCore\": 2000,\n"
//		              "            \"mem\": 2000,\n"
//		              "            \"basePrice\": 270,\n"
//		              "            \"unitPrice\": 24.55\n"
//		              "        }\n"
//		              "    ]\n"
//		              "}";
//		AlgorithmParameters p(json);
//		GAMSP g(p.convertToMipsNodes());
//		g.Run();
//		auto v = GenerateAllocations(g);
//		Array ret;
//        for(auto & microservice : v)
//        {
//        	cout<<microservice.object().json()<<endl;
//            ret << microservice.object();
//        }
//        cout<<ret.json()<<endl;


//		AlgorithmParameters p (
//				{{"node1", 2000, 2000, 300, 27.27},
//				{"node2", 2000, 1800, 240, 21.82},
//				{"node3", 2000, 2000, 270, 24.55},}
//		);
//		GAMSP g(p.convertToMipsNodes());
//		g.Run();
//		auto v = GenerateAllocations(g);
//		for(auto & ele:v) {
//			cout<<ele.object().json()<<endl;
//		}
//		auto params = getDefaultParameters();
//		ClearAlgorithmFiles();
//		testRoundRobin(params);
//		testRandom(params);
//		testKubernetes(params);
//		testGenetic(params);
    crow::SimpleApp app;
    CROW_ROUTE(app, "/ping").methods("GET"_method)([](){
	    return "pong";
    });
    CROW_ROUTE(app, "/algorithm").methods("GET"_method)([](const crow::request & req){
        string json = req.body;
        cout<<json<<endl;
        AlgorithmParameters params(json);
        GAMSP g(params.convertToMipsNodes());
        g.Run();
        auto v = GenerateAllocations(g);

        Array ret;
        for(auto & microservice : v)
        {
        	cout<<microservice.object().json()<<endl;
            ret << microservice.object();
        }
        return crow::response(200, ret.json());
    });
    app.port(8080).multithreaded().run();
    return 0;
}