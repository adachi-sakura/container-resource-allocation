#include "include/crow.h"
#include "Genetic.h"
#include "include/jsonxx.h"
using namespace std;
using namespace jsonxx;

int main() {
    crow::SimpleApp app;
    CROW_ROUTE(app, "/ping").methods("GET"_method)([](){
        return "pong";
    });
    CROW_ROUTE(app, "/algorithm").methods("GET"_method)([](const crow::request & req){
        string json = req.body;
        jsonxx::Object o;
        o.parse(json);
        AlgorithmParameters params;
        params.rq.cpu_total = o.get<Number>("cpu_rq_total");
        params.rq.mem_total = o.get<Number>("mem_rq_total");
        params.lm.cpu = o.get<Number>("cpu_lm");
        params.lm.mem = o.get<Number>("mem_lm");
        Array nodes = o.get<Array>("nodes");
        for(int i=0; i<nodes.size(); i++)
        {
            Object node = nodes.get<Object>(i);
            Node no;
            no.current_cpu = node.get<Number>("current_cpu");
            no.allocatable_cpu = node.get<Number>("allocatable_cpu");
            no.sum_cpu = node.get<Number>("sum_cpu");
            no.current_mem = node.get<Number>("current_mem");
            no.allocatable_mem = node.get<Number>("allocatable_mem");
            no.sum_mem = node.get<Number>("sum_mem");
            params.nodes.push_back(no);
        }
        Array microservices = o.get<Array>("datas");
        for(int i=0; i<microservices.size(); i++)
        {
            Object microservice = microservices.get<Object>(i);
            MicroserviceData ms;
            ms.network_receive = microservice.get<Number>("receive");
            ms.network_transmit = microservice.get<Number>("transmit");
            ms.cpuUsageTimeTotal = microservice.get<Number>("cpuUsageTime");
            ms.cpuTimeTotal = microservice.get<Number>("cpuTimeTotal");
            ms.httpRequestsCount = microservice.get<Number>("httpRequestCount");
            ms.maxMemoryUsage = microservice.get<Number>("maxMemoryUsage");
            ms.replica = microservice.get<Number>("replicas");
            ms.leastResponseTime = microservice.get<Number>("leastResponseTime");

            Array invokeServices = microservice.get<Array>("microservicesToInvoke");
            for(int j=0; j<invokeServices.size(); j++)
            {
                auto invokeService = invokeServices.get<Number>(j);
                ms.microservicesToInvoke.push_back(invokeService);
            }
            params.datas.push_back(ms);
        }
        params.entrancePoint = o.get<Number>("entrancePoint");
        params.bandwidth = o.get<Number>("bandwidth");
        params.totalTimeRequired = o.get<Number>("totalTimeRequired");
        string err;
        auto res = run(params, err);
        if (res.size() == 0)
        {
            return crow::response(400, err);
        }
        Array ret;
        for(auto & microservice : res)
        {
            Object ms_gene;
            Array genes;
            for(auto & container : microservice.Gen)
            {
                Object container_gene;
                container_gene << "loc" << container.loc;
                container_gene << "cpu" << container.cpu;
                genes << container_gene;
            }
            ms_gene << "containers" << genes;
            ms_gene << "requestMemory" << microservice.request_memory;
            ret << ms_gene;
        }
        return crow::response(200, ret.json());
    });
    app.port(11037).multithreaded().run();
    return 0;
}