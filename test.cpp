//
// Created by Administrator on 2020/4/29.
//
#include<fstream>
#include<iostream>
#include"test.h"
#include<vector>
#include<random>
#include<ctime>
using namespace std;

AlgorithmParameters getDefaultParameters()
{
    AlgorithmParameters params;
    params.datas = {
            {"", 20*1024, 20*1024, 30*60*0.2, 30*60, 1024*20, 100, 3, 0.5, {1}},
            {"", 100*1024, 80*1024, 30*60*0.8, 30*60, 20480, 500, 5, 0.5}
            //{"", 30*10240, 1*10240, 30*60*1.2, 30*60, 10240, 50, 2, 0.8}
    };
    params.nodes = {
        {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
        {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
        {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
        {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
        {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
        {"",300, 800, 2000, 5*1024, 8*1024, 16*1024}
    };
    params.bandwidth = 50*1024;  //50MB/s
    params.rq = ResourceQuota {10000, 5*1024};
    params.lm = LimitRange {800, 1024};
    params.totalTimeRequired = 0.8;
    params.entrancePoint = 0;

    return params;
}

vector<double> iterate()
{
    vector<double> fitnesses;
    fitnesses.reserve(MAXGENES);
    int iter = MAXGENES;
    do {

        eval();

        elite();

        select();

        crossover();

        mutate();

        aggregation();

        iter--;
        fitnesses.push_back(best_chrom.fitness);
    } while (iter>0);
    return fitnesses;
}

void testIteration(AlgorithmParameters& params)
{
    if(!init(params.nodes, params.datas, params.totalTimeRequired, params.entrancePoint, params.bandwidth, params.rq, params.lm))
    {
        cout<<"init failed"<<endl;
        return;
    }
    auto fitnesses = iterate();
    fstream file;
    file.open("output/maxgenes.csv", ios::app);
    file<<MAXGENES<<" ";
    for(auto fitness: fitnesses)
    {
        file<<fitness<<" ";
    }
    file<<endl;
    file.close();
}

void testMutation(AlgorithmParameters& params)
{
    if(!init(params.nodes, params.datas, params.totalTimeRequired, params.entrancePoint, params.bandwidth, params.rq, params.lm))
    {
        cout<<"init failed"<<endl;
        return;
    }
    auto fitnesses = iterate();
    fstream file;
    file.open("output/pmutation.csv", ios::app);
    file<<PMUTATION<<" ";
    for(auto fitness: fitnesses)
    {
        file<<fitness<<" ";
    }
    file<<endl;
    file.close();
}

void testCrossOver(AlgorithmParameters& params)
{
    if(!init(params.nodes, params.datas, params.totalTimeRequired, params.entrancePoint, params.bandwidth, params.rq, params.lm))
    {
        cout<<"init failed"<<endl;
        return;
    }
    auto fitnesses = iterate();
    fstream file;
    file.open("output/pxover.csv", ios::app);
    file<<PXOVER<<" ";
    for(auto fitness: fitnesses)
    {
        file<<fitness<<" ";
    }
    file<<endl;
    file.close();
}

void testElite(AlgorithmParameters& params)
{
    if(!init(params.nodes, params.datas, params.totalTimeRequired, params.entrancePoint, params.bandwidth, params.rq, params.lm))
    {
        cout<<"init failed"<<endl;
        return;
    }
    auto fitnesses = iterate();
    fstream file;
    file.open("output/elite.csv", ios::app);
    file<<ELITERATE<<" ";
    for(auto fitness: fitnesses)
    {
        file<<fitness<<" ";
    }
    file<<endl;
    file.close();
}

void testPenalty(AlgorithmParameters& params)
{
    if(!init(params.nodes, params.datas, params.totalTimeRequired, params.entrancePoint, params.bandwidth, params.rq, params.lm))
    {
        cout<<"init failed"<<endl;
        return;
    }
    auto fitnesses = iterate();
    fstream file;
    file.open("output/penalty.csv", ios::app);
    file<<PENALTYRATE<<" ";
    for(auto fitness: fitnesses)
    {
        file<<fitness<<" ";
    }
    file<<endl;
    file.close();
}

struct Pod
{
    double cpu;
    double mem;
};

vector<Pod> standardPod(AlgorithmParameters& params)
{
    vector<Pod> pods;
    for(auto const & ms : params.datas)
    {
        double mem = ms.maxMemoryUsage;
        double cpu = ms.cpuUsageTimeTotal/ms.httpRequestsCount;
        pods.push_back(Pod{cpu, mem});
    }
    return pods;
}

double calcFitness(vector<Node> & nodes, vector<Node> & originalNodes)
{
    double sum_cpu_utility = 0;
    double sum_mem_utility = 0;
    int num = 0;
    for(int i=0; i<nodes.size(); i++)
    {
        if (nodes[i].current_cpu != originalNodes[i].current_cpu)
        {
            num++;
            sum_cpu_utility += nodes[i].current_cpu/nodes[i].sum_cpu;
            sum_mem_utility += nodes[i].current_mem/nodes[i].sum_mem;
        }
    }
    return 0.5*sum_cpu_utility/num+0.5*sum_mem_utility/num;
}

void testRoundRobin(AlgorithmParameters& params)
{
    auto pods = standardPod(params);
    auto ms_total = params.datas.size();
    int nodeCount = -1;
retry:
    auto nodes = params.nodes;

    if (nodeCount == nodes.size()-1)
        throw("no enough capacity");
    nodeCount++;
    int cur = nodeCount;
    for(int i=0; i<ms_total; i++)
    {
        auto & pod = pods[i];
        for(int j=0; j<params.datas[i].replica; j++)
        {
            int first = cur;
            while(!nodes[cur].is_available(pod.cpu, pod.mem))
            {
                cur = (cur+1)%nodes.size();
                if( cur == first)
                    goto retry;
            }
            nodes[cur].allocate(pod.cpu, pod.mem);
            cur = (cur+1)%nodes.size();
        }
    }
    auto fitness = calcFitness(nodes, params.nodes);
    fstream file;
    file.open("output/algorithm.csv", ios::app);
    file<<"RoundRobin"<<" "<<fitness<<endl;
    file.close();
}

void testRandom(AlgorithmParameters& params)
{
    default_random_engine engine(time(nullptr));
    auto pm_total = params.nodes.size();
    uniform_int_distribution<int> random(0, pm_total-1);
    auto pods = standardPod(params);
    auto ms_total = params.datas.size();
retry:
    auto nodes = params.nodes;
    for(int i=0; i<ms_total; i++)
    {
        auto & pod = pods[i];
        for(int j=0; j<params.datas[i].replica; j++)
        {
            int loc = random(engine);
            if(!nodes[loc].allocate(pod.cpu, pod.mem))
                goto retry;
        }
    }
    auto fitness = calcFitness(nodes, params.nodes);
    fstream file;
    file.open("output/algorithm.csv", ios::app);
    file<<"Random"<<" "<<fitness<<endl;
    file.close();
}



