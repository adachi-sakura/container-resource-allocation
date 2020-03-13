//
// Created by Administrator on 2020/3/8.
//

#include "Generic.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <ctime>
#include <cstring>
#include <limits>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <map>
#include <random>

using namespace std;

default_random_engine engine(time(NULL));
uniform_real_distribution<double> random_probability(0., 1.);


const double BUMPUPRATIO = 1.2;
const int MINBUMPUP = 100;  //100MB
int bandwidth;    //带宽
ResourceQuota resourceQuota;
LimitRange limitRange;


vector<Node> nodes; //主机
int NODES_TOTAL;

vector<Microservice> microservices;
int MS_TOTAL; //微服务数

vector<Microservice_gene> output;     //返回结果

vector<Chromsome> popcurrent(POPSIZE);    //当代种群
Chromsome best_chrom;
vector<Chromsome> popnext(POPSIZE);       //下代种群

void Gene::init(int ms_num)
{
    uniform_int_distribution<int> random(0, NODES_TOTAL-1);
    this->loc = random(engine);
    double max_request_cpu = min(nodes[loc].available_cpu(), limitRange.cpu);//受limit range限制
    this->cpu = random_probability(engine) * (max_request_cpu - microservices[ms_num].cpu_min) + microservices[ms_num].cpu_min;
}

void Chromsome::init()
{
    Gen = vector<vector<Gene>> (MS_TOTAL);
    for(int i=0; i<MS_TOTAL; i++)
    {
        Gen[i] = vector<Gene>(microservices[i].replicas);
    }
    fitness = 0;
    rfitness = 0;
    cfitness = 0;
    do{
        for(int i=0;i<Gen.size();i++)
        {
            for(int j=0;j<Gen[i].size();j++)
            {
                Gen[i][j].init(i);
            }
        }
    }while(!restrain(Gen));
}

bool restrain(const vector<vector<Gene>> &Gen)
{
    struct resource
    {
        double cpu;
        double mem;
        resource(){cpu=0; mem=0;}
    };
    vector<resource> node_used(NODES_TOTAL);
    for(int i=0;i<Gen.size();i++)   //任一物理机，被部署在上面的所有微服务实例的计算能力之和不超过该物理机的总计算能力
    {
        for(int j=0;j<Gen[i].size();j++)
        {
            int loc = Gen[i][j].loc;
            if (loc < 0 || loc >= NODES_TOTAL)
                continue;
            node_used[loc].cpu += Gen[i][j].cpu;
            if (node_used[loc].cpu>nodes[loc].available_cpu())
                return false;
            node_used[loc].mem += microservices[i].request_memory;
            if (node_used[loc].mem>nodes[loc].available_mem())
                return false;
        }
    }

    for(int i=0;i<Gen.size();i++)   // 任一微服务容器被分配的cpu要大于等于最小cpu 小于等于最大cpu
    {
        for(int j=0;j<Gen[i].size();j++)
        {
            if(Gen[i][j].cpu < microservices[i].cpu_min || Gen[i][j].cpu > microservices[i].cpu_max_used)
                return false;
        }
    }

    resource total_used;
    for(int i=0;i<Gen.size();i++)   // 微服务实例的计算能力之和不超过ResourceQuota的总计算能力
    {
        total_used.mem += microservices[i].request_memory*microservices[i].replicas;
        if (total_used.mem > resourceQuota.mem_total)
            return false;
        for(auto container : Gen[i])
        {
            total_used.cpu += container.cpu;
            if (total_used.cpu > resourceQuota.cpu_total)
                return false;
        }
    }

    for(int i=0;i<Gen.size();i++)   // 所有微服务实例不超过Limit Range
    {
        if (microservices[i].request_memory > limitRange.mem)
            return false;
        for(auto container : Gen[i])
        {
            if(container.cpu > limitRange.cpu)
                return false;
        }
    }

    for(const auto & microservice : Gen)   // 所有微服务实例全部署
    {
        for(auto container : microservice)
        {
            if(container.loc < 0 || container.loc >= NODES_TOTAL)
                return false;
        }
    }
}

vector<bool> restrain_count(const vector<vector<Gene>> &Gen)
{
    vector<bool> ret;
    struct resource
    {
        double cpu;
        double mem;
        resource(){cpu=0; mem=0;}
    };
    vector<resource> node_used(NODES_TOTAL);
    ret.push_back(true);
    for(int i=0;i<Gen.size();i++)   //任一物理机，被部署在上面的所有微服务实例的计算能力之和不超过该物理机的总计算能力
    {
        for(int j=0;j<Gen[i].size();j++)
        {
            int loc = Gen[i][j].loc;
            if (loc < 0 || loc >= NODES_TOTAL)
                continue;
            node_used[loc].cpu += Gen[i][j].cpu;
            if (node_used[loc].cpu>nodes[loc].available_cpu())
            {
                ret.back() = false;
                goto r2;
            }

            node_used[loc].mem += microservices[i].request_memory;
            if (node_used[loc].mem>nodes[loc].available_mem())
            {
                ret.back() = false;
                goto r2;
            }
        }
    }

r2:
    ret.push_back(true);
    for(int i=0;i<Gen.size();i++)   // 任一微服务容器被分配的cpu要大于等于最小cpu 小于等于最大cpu
    {
        for(int j=0;j<Gen[i].size();j++)
        {
            if(Gen[i][j].cpu < microservices[i].cpu_min || Gen[i][j].cpu > microservices[i].cpu_max_used)
            {
                ret.back() = false;
                goto r3;
            }
        }
    }

r3:
    ret.push_back(true);
    resource total_used;
    for(int i=0;i<Gen.size();i++)   // 微服务实例的计算能力之和不超过ResourceQuota的总计算能力
    {
        total_used.mem += microservices[i].request_memory*microservices[i].replicas;
        if (total_used.mem > resourceQuota.mem_total)
        {
            ret.back() = false;
            goto r4;
        }
        for(auto container : Gen[i])
        {
            total_used.cpu += container.cpu;
            if (total_used.cpu > resourceQuota.cpu_total)
            {
                ret.back() = false;
                goto r4;
            }
        }
    }

r4:
    ret.push_back(true);
    for(int i=0;i<Gen.size();i++)   // 所有微服务实例不超过Limit Range
    {
        if (microservices[i].request_memory > limitRange.mem)
        {
            ret.back() = false;
            goto r5;
        }
        for(auto container : Gen[i])
        {
            if(container.cpu > limitRange.cpu)
            {
                ret.back() = false;
                goto r5;
            }
        }
    }

r5:
    ret.push_back(true);
    for(const auto & microservice : Gen)   // 所有微服务实例全部署
    {
        for(auto container : microservice)
        {
            if(container.loc < 0 || container.loc >= NODES_TOTAL)
            {
                ret.back() = false;
                goto end;
            }
        }
    }
end:
    return ret;
}

double Network_usage::sum()
{
    return receive+transmit;
}

double Node::available_cpu() { return sum_cpu-current_cpu;}
double Node::available_mem() { return sum_mem-current_mem;}

bool valid()//条件的合法性
{
    double ms_cpu_total=0;
    double ms_mem_total=0;
    for(int i=0;i<MS_TOTAL;i++)
    {
        double network = microservices[i].network_usage.sum();
        if(network/bandwidth+microservices[i].cpu_usage_time>microservices[i].least_response_time)//最长响应时间无法满足
            return false;

        ms_cpu_total += microservices[i].cpu_min*microservices[i].replicas;
        ms_mem_total += microservices[i].request_memory*microservices[i].replicas;
    }
    if (ms_cpu_total > resourceQuota.cpu_total || ms_mem_total > resourceQuota.mem_total)//最低总量超出了resourceQuota
        return false;

    double nodes_cpu_total=0;
    double nodes_mem_total=0;
    for(int i=0;i<NODES_TOTAL;i++)
    {
        nodes_cpu_total=nodes_cpu_total+nodes[i].available_cpu();
        nodes_mem_total=nodes_mem_total+nodes[i].available_mem();
    }
    return !(ms_cpu_total > nodes_cpu_total || ms_mem_total > nodes_mem_total);
}

bool init(vector<Node> &no,vector<MicroserviceData> &datas, int bw, ResourceQuota rq, LimitRange lr)
{
    nodes = no;
    NODES_TOTAL = nodes.size();
    microservices = vector<Microservice>(datas.size());
    MS_TOTAL = microservices.size();

    bandwidth = bw;
    resourceQuota = rq;
    limitRange = lr;

    for(int i=0; i<MS_TOTAL; i++)
    {
        microservices[i].network_usage.receive = datas[i].network_receive/datas[i].httpRequestsCount;
        microservices[i].network_usage.transmit = datas[i].network_transmit/datas[i].httpRequestsCount;
        microservices[i].least_response_time = datas[i].leastResponseTime;
        microservices[i].cpu_usage_time = datas[i].cpuUsageTimeTotal/datas[i].httpRequestsCount;
        microservices[i].cpu_max_used = datas[i].cpuUsageTimeTotal/datas[i].cpuTimeTotal;
        microservices[i].http_requests_count = datas[i].httpRequestsCount;

        /**********************************************************************************
         * cpu_usage_time*cpu_max_used/cpu_min+network/bandwidth <= least_response_time
         * cpu_usage_time*cpu_max_used/(least_response_time-network/bandwidth) <= cpu_min
         ***********************************************************************************/
        microservices[i].cpu_min = microservices[i].cpu_usage_time*microservices[i].cpu_max_used
                                   /(microservices[i].least_response_time-microservices[i].network_usage.sum()/bandwidth);
        microservices[i].max_memory_usage = datas[i].maxMemoryUsage;
        microservices[i].replicas = datas[i].replica;
    }

    calculate_mem_request();
    if(!valid())
        return false;

    for(auto & chromsome : popcurrent)
        chromsome.init();

    return true;
}

void calculate_mem_request()//根据历史最大值计算申请量
{
    for(int i=0; i<MS_TOTAL; i++)
    {
        double max_memory = microservices[i].max_memory_usage;
        double request_memory = max(max_memory+MINBUMPUP, max_memory*BUMPUPRATIO);
        microservices[i].request_memory = min(request_memory, limitRange.mem);//受limit range限制
    }
}

double func_obj(const vector<vector<Gene>> &Gen)    //部署结点的平均使用率
{
    struct resource
    {
        double cpu;
        double mem;
        resource(){cpu=0; mem=0;}
    };
    unordered_map<int, resource> resource_usages;
    for(int i=0; i<Gen.size(); i++)
    {
        for(auto container : Gen[i])
        {
            if (container.loc < 0 || container.loc >= NODES_TOTAL)
                continue;
            resource_usages[container.loc].cpu += container.cpu;
            resource_usages[container.loc].mem += microservices[i].request_memory;
        }
    }
    struct utility
    {
        double cpu;
        double mem;
    };
    vector<utility> utilities;
    for(auto it : resource_usages)
    {
        int loc = it.first;
        auto resource_usage = it.second;
        utilities.push_back(utility{
            .cpu = resource_usage.cpu+nodes[loc].current_cpu/nodes[loc].sum_cpu,
            .mem = resource_usage.mem+nodes[loc].current_mem/nodes[loc].sum_mem
        });
    }
    double sum_cpu_utility = 0;
    double sum_mem_utility = 0;
    for(auto utility : utilities)
    {
        sum_cpu_utility += utility.cpu;
        sum_mem_utility += utility.mem;
    }
    return 0.5*sum_cpu_utility/utilities.size()+0.5*sum_mem_utility/utilities.size();
}

double restrain_normalization(int i, const vector<vector<Gene>> & Gen)
{
    double value = 0;
    struct resource
    {
        double cpu;
        double mem;
        resource(){cpu=0; mem=0;}
    };
    switch (i)
    {
        case 0:         //违反单物理机资源上限
        {
            vector<resource> resources(NODES_TOTAL);
            for(int i=0; i<Gen.size(); i++)
            {
                for(auto container: Gen[i])
                {
                    if (container.loc<0 || container.loc>=NODES_TOTAL)
                        continue;
                    resources[container.loc].cpu += container.cpu;
                    resources[container.loc].mem += microservices[i].request_memory;
                }
            }
            double total_excess = 0;
            for(int i=0; i<NODES_TOTAL; i++)
            {
                resource r = resources[i];
                double cpu_excess = max(r.cpu-nodes[i].available_cpu(), 0.)/nodes[i].available_cpu();
                double mem_excess = max(r.mem-nodes[i].available_mem(), 0.)/nodes[i].available_mem();
                total_excess = total_excess + 0.5*cpu_excess + 0.5*mem_excess;
            }
            value = total_excess/NODES_TOTAL;
            break;
        }
        case 1:     //容器大于最小cpu,小于最大cpu
        {
            double cpu_deviation_total = 0;
            double container_total = 0;
            for(int i=0; i<Gen.size(); i++)
            {
                container_total += microservices[i].replicas;
                for(auto container : Gen[i])
                {
                    if (container.cpu < microservices[i].cpu_min)
                        cpu_deviation_total += (microservices[i].cpu_min-container.cpu)/microservices[i].cpu_min;
                    else if (container.cpu > microservices[i].cpu_max_used)
                        cpu_deviation_total += (container.cpu-microservices[i].cpu_max_used)/microservices[i].cpu_max_used;
                }
            }
            value = cpu_deviation_total/container_total;
            break;
        }
        case 2: //容器资源量总和不超过Resource Quota
        {
            resource r;
            for(int i=0; i<Gen.size(); i++)
            {
                r.mem += microservices[i].request_memory*microservices[i].replicas;
                for(auto container : Gen[i])
                {
                    r.cpu += container.cpu;
                }
            }
            double cpu_excess = max(r.cpu-resourceQuota.cpu_total, 0.);
            double mem_excess = max(r.mem-resourceQuota.mem_total, 0.);
            value = 0.5*cpu_excess+0.5*mem_excess;
            break;
        }
        case 3:  //容器资源量不超过Limit Range
        {
            int container_count = 0;
            double cpu_excess_total = 0;
            double mem_excess_total = 0;
            for(int i=0; i<Gen.size(); i++)
            {
                for(auto container : Gen[i])
                {
                    container_count++;
                    cpu_excess_total += max(container.cpu-limitRange.cpu, 0.);
                    mem_excess_total += max(microservices[i].request_memory-limitRange.mem, 0.);
                }
            }
            value = (cpu_excess_total/limitRange.cpu+mem_excess_total/limitRange.mem)/container_count;
            break;
        }
        case 4: //微服务实例全部署
        {
            int not_allocated_container_total = 0;
            int container_total = 0;
            for(auto & microservice : Gen)
            {
                for(auto & container: microservice)
                {
                    if (container.loc<0 || container.loc>=NODES_TOTAL || container.cpu<=0)
                        not_allocated_container_total++;
                    container_total++;
                }
            }
            double acc = 0.0001;
            if (not_allocated_container_total > 0)
                value = (not_allocated_container_total-acc)/(container_total-acc);
            else
                value = 0;
            break;
        }
        default:
            break;
    }
    return value;
}

void eval()
{
    vector<double> penaltyRate = {0.001, 0.001, 0.001, 0.001, 0.001};
    for(auto & chromsome : popcurrent)
    {
        double obj = func_obj(chromsome.Gen);
        vector<bool> disobeys = restrain_count(chromsome.Gen);
        double penalty = 0;
        for(int i=0; i<penaltyRate.size(); i++)
        {
            if (!disobeys[i])
                penalty += penaltyRate[i]*restrain_normalization(i, chromsome.Gen);
        }
        chromsome.fitness = obj-penalty+(penalty==0);
    }

}

void elite()
{
    int elite_pos=-1;
    multimap<double, int> fitness_rank; // fitness,position 按fitness自小到大排列的map
    for(int i=0; i<popcurrent.size(); i++)
    {
        if(restrain(popcurrent[i].Gen))
        {
            if (elite_pos == -1)
                elite_pos = i;
            else
                elite_pos = popcurrent[i].fitness>popcurrent[elite_pos].fitness ? i : elite_pos;
        }
        fitness_rank.insert(make_pair(popcurrent[i].fitness, i));
    }
    if (elite_pos != -1)
        best_chrom = popcurrent[elite_pos];

    auto it = fitness_rank.begin();
    for(int i=0; i<0.2*POPSIZE; i++,it++)
    {
        popcurrent[it->second] = best_chrom;
    }
}

void select()
{
    //轮盘赌
    double fitness_sum = 0;
    for(auto & chromsome : popcurrent)
        fitness_sum += chromsome.fitness;
    double cfitness = 0;
    for(auto & chromsome : popcurrent)
    {
        chromsome.rfitness = chromsome.fitness/fitness_sum;
        cfitness += chromsome.rfitness;
        chromsome.cfitness = cfitness;
    }
    for(auto & empty_chromsome : popnext)
    {
        double roulette = random_probability(engine);
        for(const auto& chromsome : popcurrent)
        {
            if (roulette < chromsome.cfitness)
            {
                empty_chromsome = chromsome;
                break;
            }
        }
    }
    popcurrent = popnext;
}

void crossover()
{
    //单点交叉
    int container_total = 0;
    for(const auto & microservice : microservices)
    {
        container_total += microservice.replicas;
    }
    uniform_int_distribution<int> random_pop(0, POPSIZE-1);
    uniform_int_distribution<int> random_container(0, container_total-1);
    for(int cnt=0; cnt<POPSIZE; cnt++)
    {
        double p = random_probability(engine);
        if (p < PXOVER)
        {
            int chrom_num1 = random_pop(engine);
            int chrom_num2 = random_pop(engine);
            int xpoint = random_container(engine)+1;  //交换xpoint个
            int container_swapped = 0;
            for(int i=0; i<popcurrent[chrom_num1].Gen.size(); i++)
            {
                int replicas = popcurrent[chrom_num1].Gen[i].size();
                if (container_swapped + replicas <= xpoint)
                {
                    container_swapped += replicas;
                    swap(popcurrent[chrom_num1].Gen[i], popcurrent[chrom_num2].Gen[i]);
                }
                else
                {
                    int remain_to_swap = xpoint - container_swapped;
                    for(int j=0; j<remain_to_swap; j++)
                        swap(popcurrent[chrom_num1].Gen[i][j], popcurrent[chrom_num2].Gen[i][j]);
                    break;
                }
            }
        }
    }
}

void mutate()
{
    uniform_int_distribution<int> random_nodes(0, NODES_TOTAL-1);
    uniform_int_distribution<int> random_tri(0, 2);
    for(auto & chrom : popcurrent)
    {
        for(int i=0; i<chrom.Gen.size();i++)
        {
            for(auto & container : chrom.Gen[i])
            {
                double p = random_probability(engine);
                if (p<PMUTATION)
                {
                    if (container.loc >= 0 && container.loc < NODES_TOTAL
                    && container.cpu >= microservices[i].cpu_min && container.cpu <= microservices[i].cpu_max_used)
                    {
                        int rand = random_tri(engine);
                        if (rand == 2)
                        {
                            container.cpu += random_probability(engine)*(nodes[container.loc].available_cpu()-container.cpu);
                        }
                        else if (rand == 1)
                        {
                            container.cpu -= random_probability(engine)*(nodes[container.loc].available_cpu()-container.cpu);
                        }
                        else
                        {
                            int prev_loc = container.loc;
                            container.loc = random_nodes(engine);
                            double scaled_cpu = container.cpu/nodes[prev_loc].available_cpu()*nodes[container.loc].available_cpu();
                            scaled_cpu = max(microservices[i].cpu_min, scaled_cpu);
                            scaled_cpu = min(microservices[i].cpu_max_used, scaled_cpu);
                            container.cpu = scaled_cpu;
                        }
                    }
                    else
                    {
                        if(container.loc<0 || container.loc >= NODES_TOTAL)
                        {
                            container.loc = random_nodes(engine);
                        }
                        if (container.cpu < microservices[i].cpu_min || container.cpu > microservices[i].cpu_max_used)
                        {
                            double max_request_cpu = min(nodes[container.loc].available_cpu(), limitRange.cpu);//受limit range限制
                            container.cpu = random_probability(engine) * (max_request_cpu - microservices[i].cpu_min) + microservices[i].cpu_min;
                        }
                    }
                }
            }
        }
    }
}

void aggregation()
{
    for(auto & chrom : popcurrent)
    {
        struct resource
        {
            double cpu;
            double mem;
            resource(){cpu=0; mem=0;}
        };
        vector<resource> resource_usages(NODES_TOTAL);
        vector<vector<pair<int,int>>> container_allocation(NODES_TOTAL);
        for(int microservice_num=0; microservice_num < chrom.Gen.size(); microservice_num++)
        {
            for(int container_num=0; container_num < chrom.Gen[microservice_num].size(); container_num++)
            {
                auto container = chrom.Gen[microservice_num][container_num];
                if (container.loc < 0 || container.loc >= NODES_TOTAL)
                    continue;
                resource_usages[container.loc].cpu += container.cpu;
                resource_usages[container.loc].mem += microservices[microservice_num].request_memory;
                container_allocation[container.loc].push_back(make_pair(microservice_num, container_num));
            }
        }
        struct utility
        {
            int node_num;
            double cpu;
            double mem;
            double weighted_average_utility() const { return 0.5*cpu+0.5*mem;}
            bool operator<(const utility & b) const
            {
                return this->weighted_average_utility() < b.weighted_average_utility();
            }
        };
        vector<utility> utilities(NODES_TOTAL);
        for(int node_num=0; node_num < resource_usages.size(); node_num++)
        {
            utilities[node_num].node_num = node_num;
            utilities[node_num].cpu = (resource_usages[node_num].cpu + nodes[node_num].current_cpu) / nodes[node_num].sum_cpu;
            utilities[node_num].mem = (resource_usages[node_num].mem + nodes[node_num].current_mem) / nodes[node_num].sum_mem;
        }
        sort(utilities.begin(), utilities.end());
        for(int utility_rank_low=0; utility_rank_low < utilities.size(); utility_rank_low++)
        {
            int migrate_from = utilities[utility_rank_low].node_num;
            for(const auto & pair : container_allocation[migrate_from])
            {
                int microservice_num = pair.first;
                int replica_num = pair.second;
                for(int utility_rank_high = utilities.size()-1; utility_rank_high>utility_rank_low; utility_rank_high--)
                {
                    int migrate_to = utilities[utility_rank_high].node_num;
                    auto & container = chrom.Gen[microservice_num][replica_num];
                    resource new_resource;
                    new_resource.cpu = resource_usages[migrate_to].cpu + container.cpu;
                    new_resource.mem = resource_usages[migrate_to].mem + microservices[microservice_num].request_memory;
                    utility new_utility{
                        .cpu = (new_resource.cpu+nodes[migrate_to].current_cpu)/nodes[migrate_to].sum_cpu,
                        .mem = (new_resource.mem+nodes[migrate_to].current_mem)/nodes[migrate_to].sum_mem
                    };
                    if (new_utility.cpu <= 1 && new_utility.mem <= 1)   //迁移合法
                    {
                        resource_usages[migrate_to] = new_resource;
                        resource_usages[migrate_from].cpu -= container.cpu;
                        resource_usages[migrate_from].mem -= microservices[microservice_num].request_memory;
                        container.loc = migrate_to;
                    }
                }
            }
        }
    }
}

void test()
{
    //todo initialize
    vector<MicroserviceData> datas;
    vector<Node> nos;
    int bw=0;
    ResourceQuota rq{};
    LimitRange lr{};
    init(nos, datas, bw, rq, lr);
    int iter = MAXGENES;
    do {
        eval();
        elite();
        select();
        crossover();
        mutate();
        aggregation();
        iter--;
    } while (iter>0);
    cout<<best_chrom.fitness<<endl;
}