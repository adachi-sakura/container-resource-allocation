//
// Created by Administrator on 2020/5/11.
//
#include "Genetic.h"
#include "include/jsonxx.h"
using namespace jsonxx;

void AlgorithmParameters::unserialize(const std::string & json)
{
    jsonxx::Object o;
    o.parse(json);
    this->rq.cpu_total = o.get<Number>("cpu_rq_total");
    this->rq.mem_total = o.get<Number>("mem_rq_total");
    this->lm.cpu = o.get<Number>("cpu_lm");
    this->lm.mem = o.get<Number>("mem_lm");
    Array nodesArr = o.get<Array>("nodes");
    using nodesIndexType = decltype(nodesArr.size());
    for(nodesIndexType i=0; i < nodesArr.size(); i++)
    {
        Object node = nodesArr.get<Object>(i);
        Node no;
        no.name = node.get<String>("name");
        no.current_cpu = node.get<Number>("current_cpu");
        no.allocatable_cpu = node.get<Number>("allocatable_cpu");
        no.sum_cpu = node.get<Number>("sum_cpu");
        no.current_mem = node.get<Number>("current_mem");
        no.allocatable_mem = node.get<Number>("allocatable_mem");
        no.sum_mem = node.get<Number>("sum_mem");
        this->nodes.push_back(no);
    }
    Array microservices = o.get<Array>("datas");
    using msIndexType = decltype(microservices.size());
    for(msIndexType i=0; i<microservices.size(); i++)
    {
        Object microservice = microservices.get<Object>(i);
        MicroserviceData ms;
        ms.name = microservice.get<String>("name");
        ms.network_receive = microservice.get<Number>("receive");
        ms.network_transmit = microservice.get<Number>("transmit");
        ms.cpuUsageTimeTotal = microservice.get<Number>("cpuUsageTime");
        ms.cpuTimeTotal = microservice.get<Number>("cpuTimeTotal");
        ms.httpRequestsCount = microservice.get<Number>("httpRequestCount");
        ms.maxMemoryUsage = microservice.get<Number>("maxMemoryUsage");
        ms.replica = microservice.get<Number>("replicas");
        ms.leastResponseTime = microservice.get<Number>("leastResponseTime");

        Array invokeServices = microservice.get<Array>("microservicesToInvoke");
        using invokeServicesIndexType = decltype(invokeServices.size());
        for(invokeServicesIndexType j=0; j<invokeServices.size(); j++)
        {
            auto invokeService = invokeServices.get<Number>(j);
            ms.microservicesToInvoke.push_back(invokeService);
        }
        this->datas.push_back(ms);
    }
    this->entrancePoint = o.get<Number>("entrancePoint");
    this->bandwidth = o.get<Number>("bandwidth");
    this->totalTimeRequired = o.get<Number>("totalTimeRequired");
}

AlgorithmParameters::AlgorithmParameters(std::string &json)
{
    unserialize(json);
}

jsonxx::Object Microservice_gene::object() {
    Object ms_gene;
    ms_gene << "name" << this->name;
    Array genes;
    for(auto & container : this->Gen)
    {
        Object container_gene;
        container_gene << "loc" << container.loc;
        container_gene << "cpu" << container.cpu;
        genes << container_gene;
    }
    ms_gene << "containers" << genes;
    ms_gene << "requestMemory" << this->request_memory;
    return ms_gene;
}