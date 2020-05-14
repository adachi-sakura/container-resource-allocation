//
// Created by Administrator on 2020/3/8.
//
#ifndef GENERIC_ALGORITHM_GENETIC_H
#define GENERIC_ALGORITHM_GENETIC_H

#include <vector>
#include <string>
#include "Type.h"

#define POPSIZE  200             //个体数

#define MAXGENES  200            //最大迭代次数

#define PXOVER  0.9            //交叉概率

#define PMUTATION 0.25          //变异概率

#define ELITERATE 0.3

#define RESTRAINNUM 6
#define PENALTYRATE 0.1

struct ResourceQuota
{
    double cpu_total;
    double mem_total;
};

struct LimitRange
{
    double cpu;
    double mem;
};

struct Gene
{
    int loc;
    double cpu;
    void init(int ms_num);
};

struct Microservice_gene: SerializableJSONObject
{
    std::string name;
    struct allocation
    {
        std::string loc;
        double cpu;
    };
    std::vector<allocation> Gen;
    double request_memory;
    jsonxx::Object object() override ;
};

using MicroserviceGenes = std::vector<std::vector<Gene>>;
struct Chromosome
{
    MicroserviceGenes Gen;   //存放MS_TOTAL个微服务下replica个实例的资源分配
    double fitness=0;
    double rfitness=0;
    double cfitness=0;
    void init();
    void print();
};

struct Node
{
    std::string name;
    double current_cpu;
    double allocatable_cpu;
    double sum_cpu;
    double current_mem;
    double allocatable_mem;
    double sum_mem;
    double available_cpu();
    double available_mem();
    bool is_available(double, double);
    bool allocate(double, double);
};

struct Network_usage
{
    double receive;
    double transmit;
    double sum() const;
};

struct Microservice
{
    std::string name;
    Network_usage network_usage; //每个请求使用网络流量
    double max_response_time; //最长响应时间
    double cpu_usage_time;  //单条指令占用的cpu时间
    double cpu_min;         //最低需要的cpu
    double cpu_max_used;  //试运行的cpu占用
    double max_memory_usage;       //内存峰值
    int http_requests_count;    //请求数
    double request_memory;     //内存申请量
    int replicas;   //实例数
    std::vector<int> next_microservices;
};

struct MicroserviceData
{
    std::string name;
    int network_receive;
    int network_transmit;
    double cpuUsageTimeTotal;
    double cpuTimeTotal;
    int httpRequestsCount;
    double maxMemoryUsage;
    int replica;
    double leastResponseTime;
    std::vector<int> microservicesToInvoke;
};

struct AlgorithmParameters: SerializableJSONObject
{
    ResourceQuota rq{};
    LimitRange lm{};
    std::vector<Node> nodes;
    std::vector<MicroserviceData> datas;
    int entrancePoint = 0;
    int bandwidth = 0;
    double totalTimeRequired = 0;
    void unserialize(const std::string &) override ;
    explicit AlgorithmParameters(std::string &);
    AlgorithmParameters()= default;
};

void calculate_mem_request();//根据历史最大值计算申请量
bool init(std::vector<Node> &no,std::vector<MicroserviceData> &datas, double totalTimeRequire, int entrancePoint, int bw, ResourceQuota rq, LimitRange lr);
bool valid();//条件的合法性
bool restrain(const std::vector<std::vector<Gene>> &Gen); //约束函数
std::vector<bool> restrain_count(const std::vector<std::vector<Gene>> &Gen); //计算违反约束
double func_obj(const std::vector<std::vector<Gene>> &Gen); //目标函数
double restrain_normalization(int i, const std::vector<std::vector<Gene>> &Gen );
void eval();    //计算适应度函数
void elite();   //留存精英
void select();  //选择
void crossover();   //交叉
void mutate();  //变异
void aggregation(); //聚合
void test();
bool checkLoopDependency(std::vector<bool> &route, std::vector<bool> & checked, std::vector<Microservice> & microservices, int entrance);
double calServiceResponseTime(const std::vector<Microservice> & micro_services, const std::vector<std::vector<Gene>> & Gen, int entry, int depth);
double calBestResponseTime(const std::vector<Microservice> & micro_services, int entry, int depth);
std::vector<Microservice_gene>& run(AlgorithmParameters &, std::string &);
int AllocatedNodesNum(const MicroserviceGenes &);
#endif //GENERIC_ALGORITHM_GENETIC_H
