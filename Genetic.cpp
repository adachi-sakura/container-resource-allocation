//
// Created by Administrator on 2020/3/8.
//

#include "Genetic.h"
#include <iostream>
#include <ctime>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <map>
#include <random>

using namespace std;


default_random_engine engine(time(nullptr));
uniform_real_distribution<double> random_probability(0., 1.);


const double BUMPUPRATIO = 1.2;
const double MINBUMPUP = 50;  //50MB
int bandwidth;    //带宽
ResourceQuota resourceQuota;
LimitRange limitRange;


vector<Node> nodes; //主机
int NODES_TOTAL;

vector<Microservice> microservices;
int MS_TOTAL; //微服务数

vector<Microservice_gene> output;     //返回结果

vector<Chromosome> popcurrent(POPSIZE);    //当代种群
Chromosome best_chrom;
vector<Chromosome> popnext(POPSIZE);       //下代种群
double max_totalTime;
int execution_entrance;

void Gene::init(int ms_num)
{
    uniform_int_distribution<int> random(0, NODES_TOTAL-1);
    this->loc = random(engine);
    double max_request_cpu = min(min(nodes[loc].available_cpu(), limitRange.cpu), microservices[ms_num].cpu_max_used);//受limit range限制
    this->cpu = random_probability(engine) * (max_request_cpu - microservices[ms_num].cpu_min) + microservices[ms_num].cpu_min;
}

void Chromosome::init()
{
    Gen = MicroserviceGenes (MS_TOTAL);
    for(int i=0; i<MS_TOTAL; i++)
    {
        Gen[i] = vector<Gene>(microservices[i].replicas);
    }
    fitness = 0;
    rfitness = 0;
    cfitness = 0;
    using GeneIndexType = decltype(Gen.size());
    do{
        for(GeneIndexType i=0;i<Gen.size();i++)
        {
            for(GeneIndexType j=0;j<Gen[i].size();j++)
            {
                Gen[i][j].init(i);
            }
        }
    }while(!restrain(Gen));
}

void Chromosome::print() {
    cout<<"Chrom fitness is: "<<this->fitness<<endl;
    using GeneIndexType = decltype(Gen.size());
    for(GeneIndexType i=0; i<Gen.size(); i++)
    {
        cout<<"microservice "<<i<<endl;
        cout<<"memory request: "<<microservices[i].request_memory<<endl;
        for(auto & container : Gen[i])
        {
            cout<<"    loc: "<<container.loc<<"  cpu: "<<container.cpu<<endl;
        }
    }
}

double Chromosome::SumCpu() {
	double sum = 0;
	for(const auto & ms: Gen)
	{
		for(const auto & container: ms)
		{
			sum += container.cpu;
		}
	}
	return sum;
}

bool restrain(const MicroserviceGenes &Gen)
{
    struct resource
    {
        double cpu;
        double mem;
        resource(){cpu=0; mem=0;}
    };
    vector<resource> node_used(NODES_TOTAL);
    using GeneIndexType = decltype(Gen.size());
    for(GeneIndexType i=0;i<Gen.size();i++)   //任一物理机，被部署在上面的所有微服务实例的计算能力之和不超过该物理机的总计算能力
    {
        for(GeneIndexType j=0;j<Gen[i].size();j++)
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
    //cout<<"r1 passed"<<endl;

    for(GeneIndexType i=0;i<Gen.size();i++)   // 任一微服务容器被分配的cpu要大于等于最小cpu 小于等于最大cpu
    {
        for(GeneIndexType j=0;j<Gen[i].size();j++)
        {
            if(Gen[i][j].cpu < microservices[i].cpu_min || Gen[i][j].cpu > microservices[i].cpu_max_used)
                return false;
        }
    }
    //cout<<"r2 passed"<<endl;

    resource total_used;
    for(GeneIndexType i=0;i<Gen.size();i++)   // 微服务实例的计算能力之和不超过ResourceQuota的总计算能力
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
    //cout<<"r3 passed"<<endl;

    for(GeneIndexType i=0;i<Gen.size();i++)   // 所有微服务实例不超过Limit Range
    {
        if (microservices[i].request_memory > limitRange.mem)
            return false;
        for(auto container : Gen[i])
        {
            if(container.cpu > limitRange.cpu)
                return false;
        }
    }
    //cout<<"r4 passed"<<endl;

    for(const auto & microservice : Gen)   // 所有微服务实例全部署
    {
        for(auto container : microservice)
        {
            if(container.loc < 0 || container.loc >= NODES_TOTAL)
                return false;
        }
    }
    //cout<<"r5 passed"<<endl;

    double total_response_time = calServiceResponseTime(microservices, Gen, execution_entrance, 0);
    if(total_response_time > max_totalTime)
        return false;
    //cout<<"restrain all passed"<<endl;

    return true;
}

double calServiceResponseTime(const vector<Microservice> & micro_services, const MicroserviceGenes & Gen, int entry, int depth)
{
    double response_time = 0;
    double cpu_time=0;
    for(const auto & container : Gen[entry])
    {
        Microservice microservice = micro_services[entry];
        cpu_time = max(cpu_time, microservice.cpu_usage_time*microservice.cpu_max_used/container.cpu);
    }
    response_time += cpu_time;
    if(depth%2 == 0)
        response_time += micro_services[entry].network_usage.sum()/bandwidth;
    for(auto microservice_num : micro_services[entry].next_microservices)
    {
        response_time += calServiceResponseTime(micro_services, Gen, microservice_num, depth+1);
    }
    return response_time;
}

double calBestResponseTime(const vector<Microservice> & micro_services, int entry, int depth)
{
    double response_time = micro_services[entry].cpu_usage_time;
    if(depth%2 == 0)
        response_time += micro_services[entry].network_usage.sum()/bandwidth;
    for(auto microservice_num : micro_services[entry].next_microservices)
    {
        response_time += calBestResponseTime(micro_services, microservice_num, depth+1);
    }
    return response_time;
}

vector<bool> restrain_count(const MicroserviceGenes &Gen)
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
    using GeneIndexType = decltype(Gen.size());
    for(GeneIndexType i=0;i<Gen.size();i++)   //任一物理机，被部署在上面的所有微服务实例的计算能力之和不超过该物理机的总计算能力
    {
        for(GeneIndexType j=0;j<Gen[i].size();j++)
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
    for(GeneIndexType i=0;i<Gen.size();i++)   // 任一微服务容器被分配的cpu要大于等于最小cpu 小于等于最大cpu
    {
        for(GeneIndexType j=0;j<Gen[i].size();j++)
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
    for(GeneIndexType i=0;i<Gen.size();i++)   // 微服务实例的计算能力之和不超过ResourceQuota的总计算能力
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
    for(GeneIndexType i=0;i<Gen.size();i++)   // 所有微服务实例不超过Limit Range
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

double Network_usage::sum() const
{
    return receive+transmit;
}

double Node::available_cpu() { return allocatable_cpu;}
double Node::available_mem() { return allocatable_mem;}
bool Node::is_available(double requested_cpu, double requested_mem)
{
    return requested_cpu<= available_cpu() && requested_mem <= available_mem();
}

bool Node::allocate(double requested_cpu, double requested_mem)
{
    if(!is_available(requested_cpu, requested_mem))
        return false;
    allocatable_cpu -= requested_cpu;
    allocatable_mem -= requested_mem;
    current_cpu += requested_cpu;
    current_mem += requested_mem;
    return true;
}

bool valid()//条件的合法性
{
    double ms_cpu_total=0;
    double ms_mem_total=0;
    for(int i=0;i<MS_TOTAL;i++)
    {
        double network = microservices[i].network_usage.sum();
        if(network/bandwidth+microservices[i].cpu_usage_time>microservices[i].max_response_time)//最长响应时间无法满足
        {
            cout<<"microservice "<<i<<" least time invalid"<<endl;
            return false;
        }

        ms_cpu_total += microservices[i].cpu_min*microservices[i].replicas;
        ms_mem_total += microservices[i].request_memory*microservices[i].replicas;
    }
    if (ms_cpu_total > resourceQuota.cpu_total || ms_mem_total > resourceQuota.mem_total)//最低总量超出了resourceQuota
    {
        cout<<"resource quota invalid"<<endl;
        return false;
    }

    double nodes_cpu_total=0;
    double nodes_mem_total=0;
    for(int i=0;i<NODES_TOTAL;i++)
    {
        nodes_cpu_total=nodes_cpu_total+nodes[i].available_cpu();
        nodes_mem_total=nodes_mem_total+nodes[i].available_mem();
    }
    if(ms_cpu_total > nodes_cpu_total || ms_mem_total > nodes_mem_total)
    {
        cout<<"nodes resource invalid"<<endl;
        return false;
    }

    vector<bool> route(microservices.size(), false);
    vector<bool> checked(microservices.size(), false);
    if(!checkLoopDependency(route, checked, microservices, execution_entrance))
    {
        cout<<"loop dependency found"<<endl;
        return false;
    }

    if(calBestResponseTime(microservices, execution_entrance, 0) > max_totalTime)
    {
        cout<<"max_total_time invalid"<<endl;
        return false;
    }
    return true;
}

bool init(vector<Node> &no,vector<MicroserviceData> &datas, double totalTimeRequire, int entrancePoint, int bw, ResourceQuota rq, LimitRange lr)
{
    nodes = no;
    NODES_TOTAL = nodes.size();
    microservices = vector<Microservice>(datas.size());
    MS_TOTAL = microservices.size();

    bandwidth = bw;
    resourceQuota = rq;
    limitRange = lr;
    execution_entrance = entrancePoint;
    max_totalTime = totalTimeRequire;

    for(int i=0; i<MS_TOTAL; i++)
    {
        microservices[i].name = datas[i].name;
        microservices[i].network_usage.receive = static_cast<float>(datas[i].network_receive)/static_cast<float>(datas[i].httpRequestsCount);
        microservices[i].network_usage.transmit = static_cast<float>(datas[i].network_transmit)/static_cast<float>(datas[i].httpRequestsCount);
        microservices[i].max_response_time = datas[i].leastResponseTime;
        microservices[i].cpu_usage_time = datas[i].cpuUsageTimeTotal/datas[i].httpRequestsCount;
        microservices[i].cpu_max_used = datas[i].cpuUsageTimeTotal/datas[i].cpuTimeTotal*1000;
        microservices[i].http_requests_count = datas[i].httpRequestsCount;
        microservices[i].next_microservices = datas[i].microservicesToInvoke;

        /**********************************************************************************
         * cpu_usage_time*cpu_max_used/cpu_min+network/bandwidth <= least_response_time
         * cpu_usage_time*cpu_max_used/(least_response_time-network/bandwidth) <= cpu_min
         ***********************************************************************************/
        microservices[i].cpu_min = microservices[i].cpu_usage_time*microservices[i].cpu_max_used
                                   /(microservices[i].max_response_time - microservices[i].network_usage.sum() / bandwidth);
        microservices[i].max_memory_usage = datas[i].maxMemoryUsage;
        microservices[i].replicas = datas[i].replica;
    }

    calculate_mem_request();
    if(!valid())
        return false;

    clock_t begin,end;
    cout<<"ready to initial chromsome"<<endl;
    int cnt = 0;
    begin = clock();
    for(auto & chromsome : popcurrent)
    {
        chromsome.init();
        if(++cnt%10 == 0)
            cout<<cnt<<" initialized"<<endl;
        //cout<<"initialized"<<endl;
    }
    end = clock();
    cout<<"chromsome initialization finished, time elapsed: "<<(double)(end-begin)/CLOCKS_PER_SEC<<endl;

    return true;
}

void calculate_mem_request()//根据历史最大值计算申请量
{
    for(int i=0; i<MS_TOTAL; i++)
    {
        double max_memory = microservices[i].max_memory_usage;
        double request_memory = max(max_memory+MINBUMPUP/microservices[i].replicas, max_memory*BUMPUPRATIO);
        microservices[i].request_memory = min(request_memory, limitRange.mem);//受limit range限制
    }
}

double func_obj(const MicroserviceGenes &Gen)    //部署结点的平均使用率
{
    struct resource
    {
        double cpu;
        double mem;
        resource(){cpu=0; mem=0;}
    };
    unordered_map<int, resource> resource_usages;
    using GeneIndexType = decltype(Gen.size());
    for(GeneIndexType i=0; i<Gen.size(); i++)
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
            .cpu = (resource_usage.cpu+nodes[loc].current_cpu)/nodes[loc].sum_cpu,
            .mem = (resource_usage.mem+nodes[loc].current_mem)/nodes[loc].sum_mem
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

double restrain_normalization(int n, const MicroserviceGenes & Gen)
{
    double value = 0;
    struct resource
    {
        double cpu;
        double mem;
        resource(){cpu=0; mem=0;}
    };
    using GeneIndexType = decltype(Gen.size());
    switch (n)
    {
        case 0:         //违反单物理机资源上限
        {
            vector<resource> resources(NODES_TOTAL);
            for(GeneIndexType i=0; i<Gen.size(); i++)
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
            for(GeneIndexType i=0; i<NODES_TOTAL; i++)
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
            for(GeneIndexType i=0; i<Gen.size(); i++)
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
            for(GeneIndexType i=0; i<Gen.size(); i++)
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
            for(GeneIndexType i=0; i<Gen.size(); i++)
            {
                for(auto container : Gen[i])
                {
                    container_count++;
                    cpu_excess_total += max(container.cpu-limitRange.cpu, 0.);
                    mem_excess_total += max(microservices[i].request_memory-limitRange.mem, 0.);
                }
            }
            value = (0.5*cpu_excess_total/limitRange.cpu+0.5*mem_excess_total/limitRange.mem)/container_count;
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
        case 5: //不超过总响应时间
        {
            double total_time = calServiceResponseTime(microservices, Gen, execution_entrance, 0);
            value = total_time > max_totalTime ? (total_time-max_totalTime)/max_totalTime : 0;
            break;
        }
        default:
            break;
    }
    return value;
}

void eval()
{
    vector<double> penaltyRate = vector<double>(RESTRAINNUM, PENALTYRATE);
    for(auto & chromsome : popcurrent)
    {
        double obj = func_obj(chromsome.Gen);
        vector<bool> disobeys = restrain_count(chromsome.Gen);
        double penalty = 0;
        using penaltyRateIndexType = decltype(penaltyRate.size());
        for(penaltyRateIndexType i=0; i<penaltyRate.size(); i++)
        {
            if (!disobeys[i])
                penalty += penaltyRate[i]*restrain_normalization(i, chromsome.Gen);
        }
        chromsome.fitness = max(obj-penalty, double(0))+(penalty==0);
    }

}

void elite()
{
    using popcurrentIndexType = decltype(popcurrent.size());
    popcurrentIndexType elite_pos=-1;
    multimap<double, int> fitness_rank; // fitness,position 按fitness自小到大排列的map
    for(popcurrentIndexType i=0; i<popcurrent.size(); i++)
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
    if (elite_pos == -1)
        cout<<"bad chromosome!"<<endl;

    auto it = fitness_rank.begin();
    for(int i=0; i<ELITERATE*POPSIZE; i++,it++)
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
            using GeneIndexType = decltype(Chromosome::Gen.size());
            for(GeneIndexType i=0; i<popcurrent[chrom_num1].Gen.size(); i++)
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
    using GeneIndexType = decltype(Chromosome::Gen.size());
    for(auto & chrom : popcurrent)
    {
        for(GeneIndexType i=0; i<chrom.Gen.size();i++)
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
                            double scaled_cpu = container.cpu/(nodes[prev_loc].available_cpu()+container.cpu)*nodes[container.loc].available_cpu();
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
                            double max_request_cpu = min(min(nodes[container.loc].available_cpu(), limitRange.cpu), microservices[i].cpu_max_used);//受limit range限制
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
        using GeneIndexType = decltype(chrom.Gen.size());
        for(GeneIndexType microservice_num=0; microservice_num < chrom.Gen.size(); microservice_num++)
        {
            for(GeneIndexType container_num=0; container_num < chrom.Gen[microservice_num].size(); container_num++)
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
        using resourceUsagesIndexType = decltype(resource_usages.size());
        for(resourceUsagesIndexType node_num=0; node_num < resource_usages.size(); node_num++)
        {
            utilities[node_num].node_num = node_num;
            utilities[node_num].cpu = (resource_usages[node_num].cpu + nodes[node_num].current_cpu) / nodes[node_num].sum_cpu;
            utilities[node_num].mem = (resource_usages[node_num].mem + nodes[node_num].current_mem) / nodes[node_num].sum_mem;
        }
        sort(utilities.begin(), utilities.end());
        using utilitiesIndexType = decltype(utilities.size());
        for(utilitiesIndexType utility_rank_low=0; utility_rank_low < utilities.size(); utility_rank_low++)
        {
            int migrate_from = utilities[utility_rank_low].node_num;
            for(const auto & pair : container_allocation[migrate_from])
            {
                int microservice_num = pair.first;
                int replica_num = pair.second;
                for(utilitiesIndexType utility_rank_high = utilities.size()-1; utility_rank_high>utility_rank_low; utility_rank_high--)
                {
                    int migrate_to = utilities[utility_rank_high].node_num;
                    auto & container = chrom.Gen[microservice_num][replica_num];
                    resource new_resource;
                    new_resource.cpu = resource_usages[migrate_to].cpu + container.cpu;
                    new_resource.mem = resource_usages[migrate_to].mem + microservices[microservice_num].request_memory;
                    if (new_resource.cpu <= nodes[migrate_to].allocatable_cpu && new_resource.mem <= nodes[migrate_to].allocatable_mem)   //迁移合法
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
    vector<MicroserviceData> datas = {
		    {"", 20*1024, 20*1024, 30*60*0.2, 30*60, 1024*20, 100, 3, 0.5, {1}},
		    {"", 100*1024, 80*1024, 30*60*0.8, 30*60, 20480, 500, 5, 0.5},
		    {"", 30*10240, 1*10240, 30*60*1.2, 30*60, 10240, 50, 2, 0.8},
		    {"", 100*1024, 80*1024, 30*60*0.8, 30*60, 20480, 500, 5, 0.5, {1, 2}},
    };
    vector<Node> nos = {
		    {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
		    {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
		    {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
		    {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
		    {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
		    {"",300, 800, 2000, 5*1024, 8*1024, 16*1024},
		    {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
		    {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
    };
    int bw = 50*1024;  //50MB/s
    ResourceQuota rq{10000, 10*1024};
    LimitRange lr{800, 1024};
    double total = 10;
    int entrance = 0;
    clock_t begin,end;
    begin = clock();
    if (!init(nos, datas, total, entrance, bw, rq, lr))
    {
        cout<<"init failed"<<endl;
        return;
    }
    int iter = MAXGENES;
    cout<<"test begin"<<endl;
    do {

        eval();

        elite();

        select();

        crossover();

        mutate();

        aggregation();

        iter--;
        cout<<"iter: "<<MAXGENES-iter+1<<"  "<<best_chrom.fitness<<endl;
    } while (iter>0);
    end = clock();
    cout<<"test finished, total time elapsed: "<<(double)(end-begin)/CLOCKS_PER_SEC<<endl;
    best_chrom.print();
}

bool checkLoopDependency(vector<bool> &route, vector<bool> &checked, vector<Microservice> & ms,
                         int entrance)
{
    if(checked[entrance])
        return true;
    if(route[entrance])
        return false;
    route[entrance] = true;
    for(auto & nextService : ms[entrance].next_microservices)
    {
        if(!checkLoopDependency(route, checked, ms, nextService))
            return false;
    }
    route[entrance] = false;
    checked[entrance] = true;
    return true;
}

vector<Microservice_gene>& run(AlgorithmParameters & params, string & error)
{

    vector<MicroserviceData> datas = params.datas;
    vector<Node> nos = params.nodes;
    int bw = params.bandwidth;
    ResourceQuota rq = params.rq;
    LimitRange lr = params.lm;
    double total = params.totalTimeRequired;
    int entrance = params.entrancePoint;
    clock_t begin,end;
    begin = clock();
    if (!init(nos, datas, total, entrance, bw, rq, lr))
    {
        error = "initial failed";
        return output;
    }
    output = vector<Microservice_gene>(MS_TOTAL);
    int iter = MAXGENES;
    cout<<"begin"<<endl;
    do {
        cout<<"iter: "<<MAXGENES-iter+1<<endl;

        eval();

        elite();

        select();

        crossover();

        mutate();

        aggregation();

        iter--;
        cout<<best_chrom.fitness<<endl;
    } while (iter>0);
    end = clock();
    cout<<"finished, total time elapsed: "<<(double)(end-begin)/CLOCKS_PER_SEC<<endl;
    using outputIndexType = decltype(output.size());
    for(outputIndexType i=0; i<output.size(); i++)
    {
        output[i].name = microservices[i].name;
        for(auto & container : best_chrom.Gen[i])
        {
            Microservice_gene::allocation alloc;
            alloc.loc = nodes[container.loc].name;
            alloc.cpu = container.cpu;
            output[i].Gen.push_back(alloc);
        }
        output[i].request_memory = microservices[i].request_memory;
    }
    return output;
}




int AllocatedNodesNum(const MicroserviceGenes &Gen)
{
    unordered_map<int,int> allocation2NodeCountMap;
    for(auto & ms : Gen)
    {
        for(auto & gen : ms)
        {
            allocation2NodeCountMap[gen.loc]++;
        }
    }
    return allocation2NodeCountMap.size();
}