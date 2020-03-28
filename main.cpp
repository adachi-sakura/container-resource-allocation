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
        cout<<"cpu_rq_total"<<endl;
        params.rq.mem_total = o.get<Number>("mem_rq_total");
        cout<<"mem_rq_total"<<endl;
        params.lm.cpu = o.get<Number>("cpu_lm");
        cout<<"cpu_lm"<<endl;
        params.lm.mem = o.get<Number>("mem_lm");
        cout<<"mem_lm"<<endl;
        Array nodes = o.get<Array>("nodes");
        cout<<"nodes"<<endl;
        for(int i=0; i<nodes.size(); i++)
        {
            Object node = nodes.get<Object>(i);
            cout<<"node"+i<<endl;
            Node no;
            no.current_cpu = node.get<Number>("current_cpu");
            cout<<"current_cpu"<<endl;
            no.allocatable_cpu = node.get<Number>("allocatable_cpu");
            cout<<"allocatable_cpu"<<endl;
            no.sum_cpu = node.get<Number>("sum_cpu");
            cout<<"sum_cpu"<<endl;
            no.current_mem = node.get<Number>("current_mem");
            cout<<"current_mem"<<endl;
            no.allocatable_mem = node.get<Number>("allocatable_mem");
            cout<<"allocatable_mem"<<endl;
            no.sum_mem = node.get<Number>("sum_mem");
            cout<<"sum_mem"<<endl;
            params.nodes.push_back(no);
        }
        Array microservices = o.get<Array>("datas");
        cout<<"datas"<<endl;
        for(int i=0; i<microservices.size(); i++)
        {
            Object microservice = microservices.get<Object>(i);
            cout<<"microservice"<<endl;
            MicroserviceData ms;
            ms.network_receive = microservice.get<Number>("receive");
            cout<<"receive"<<endl;
            ms.network_transmit = microservice.get<Number>("transmit");
            cout<<"transmit"<<endl;
            ms.cpuUsageTimeTotal = microservice.get<Number>("cpuUsageTime");
            cout<<"cpuUsageTime"<<endl;
            ms.cpuTimeTotal = microservice.get<Number>("cpuTimeTotal");
            cout<<"cpuTimeTotal"<<endl;
            ms.httpRequestsCount = microservice.get<Number>("httpRequestCount");
            cout<<"httpRequestCount"<<endl;
            ms.maxMemoryUsage = microservice.get<Number>("maxMemoryUsage");
            cout<<"maxMemoryUsage"<<endl;
            ms.replica = microservice.get<Number>("replicas");
            cout<<"replicas"<<endl;
            ms.leastResponseTime = microservice.get<Number>("leastResponseTime");
            cout<<"leastResponseTime"<<endl;

            Array invokeServices = microservice.get<Array>("microservicesToInvoke");
            cout<<"microservicesToInvoke"<<endl;
            for(int j=0; j<invokeServices.size(); j++)
            {
                auto invokeService = invokeServices.get<Number>(j);
                cout<<"invoke services"<<endl;
                ms.microservicesToInvoke.push_back(invokeService);
            }
            params.datas.push_back(ms);
        }
        params.entrancePoint = o.get<Number>("entrancePoint");
        cout<<"entrancePoint"<<endl;
        params.bandwidth = o.get<Number>("bandwidth");
        cout<<"bandwidth"<<endl;
        params.totalTimeRequired = o.get<Number>("totalTimeRequired");
        cout<<"totalTimeRequired"<<endl;
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