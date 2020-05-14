//
// Created by Administrator on 2020/4/29.
//
#include<fstream>
#include<iostream>
#include"test.h"
#include<vector>
#include<random>
#include<ctime>
#include <map>
using namespace std;

AlgorithmParameters getDefaultParameters()
{
    AlgorithmParameters params;
    params.datas = {
            {"", 20*1024, 20*1024, 30*60*0.2, 30*60, 1024*20, 100, 3, 0.5, {1}},
            {"", 100*1024, 80*1024, 30*60*0.8, 30*60, 20480, 500, 5, 0.5},
            {"", 30*10240, 1*10240, 30*60*1.2, 30*60, 10240, 50, 2, 0.8},
            {"", 100*1024, 80*1024, 30*60*0.8, 30*60, 20480, 500, 5, 0.5, {1, 2}},
    };
    params.nodes = {
        {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
        {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
        {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
        {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
        {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
        {"",300, 800, 2000, 5*1024, 8*1024, 16*1024},
        {"",1200, 800, 2000, 10*1024, 6*1024,16*1024},
        {"",300, 700, 1000, 4*1024, 4*1024, 8*1024},
    };
    params.bandwidth = 50*1024;  //50MB/s
    params.rq = ResourceQuota {10000, 10*1024};
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
    file.open(GenerationTestOutputPath, ios::app);
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
    file.open(MutationTestOutputPath, ios::app);
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
    file.open(CrossOverTestOutputPath, ios::app);
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
    file.open(EliteTestOutputPath, ios::app);
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
    file.open(PenaltyTestOutputPath, ios::app);
    file<<PENALTYRATE<<" ";
    for(auto fitness: fitnesses)
    {
        file<<fitness<<" ";
    }
    file<<endl;
    file.close();
}



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

int calcNodesCount(vector<Node> & nodes, vector<Node> & originalNodes)
{
    int count = 0;
    for(int i = 0; i < nodes.size(); i++)
    {
        if (nodes[i].current_cpu != originalNodes[i].current_cpu)
            count++;
    }
    return count;
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
    file.open(AlgorithmFitnessTestOutputPath, ios::app);
    file<<"RoundRobin"<<" "<<fitness<<endl;
    file.close();
    file.open(AlgorithmNodeCountTestOutputPath, ios::app);
    file<<"RoundRobin"<<" "<<calcNodesCount(nodes, params.nodes)<<endl;
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
    auto nodesCount = calcNodesCount(nodes, params.nodes);
    fstream file;
    file.open(AlgorithmFitnessTestOutputPath, ios::app);
    file<<"Random"<<" "<<fitness<<endl;
    file.close();
    file.open(AlgorithmNodeCountTestOutputPath, ios::app);
    file<<"Random"<<" "<<calcNodesCount(nodes, params.nodes)<<endl;
    file.close();
}

vector<int> calcLeastAllocated(Pod& pod, vector<Node>& nodes)
{
    vector<int> ret;
    for(auto & node : nodes)
    {
        auto cpuRatio = (node.allocatable_cpu-pod.cpu)*MAXNODESCORE/node.sum_cpu;
        auto memRatio = (node.allocatable_mem-pod.mem)*MAXNODESCORE/node.sum_mem;
        ret.push_back((cpuRatio+memRatio)/2);
    }
    return ret;
}

vector<int> calcBalancedAllocation(Pod& pod, vector<Node>& nodes)
{
    vector<int> ret;
    for(auto & node: nodes)
    {
        auto cpuFraction = (node.sum_cpu-node.allocatable_cpu+pod.cpu)/node.sum_cpu;
        auto memFraction = (node.sum_mem-node.allocatable_mem+pod.mem)/node.sum_mem;
        auto mean = (cpuFraction+memFraction)/2;
        auto variance = ((cpuFraction-mean)*(cpuFraction-mean)+(memFraction-mean)*(memFraction-mean))/2;
        ret.push_back((1-variance)*MAXNODESCORE);
    }
    return ret;
}

vector<int> calcPodTopology(vector<Node>& nodes, vector<int>& locs)
{
    vector<int> distribution(nodes.size(),0);
    for(auto loc: locs)
        distribution[loc]++;
    int maxCount=0;
    for(auto cnt : distribution)
        maxCount = cnt > maxCount ? cnt:maxCount;
    vector<int> ret;
    for(auto cnt : distribution)
    {
        auto score = MAXNODESCORE;
        if (maxCount > 0)
        {
            score = MAXNODESCORE*(maxCount-cnt)/double(maxCount);
        }
        ret.push_back(score);
    }
    return ret;

}

template<typename T>
vector<T> totalScore(initializer_list<vector<T>>& l)
{
    vector<int> ret;
    for(auto item : l)
    {
        if(ret.size() > item.size())
            throw("length invalid");
        if(ret.size() < item.size())
            ret.resize(item.size());
        for(int i=0 ; i<item.size(); i++)
        {
            ret[i] += item[i];
        }
    }
    return ret;
}

void testKubernetes(AlgorithmParameters& params)
{
    auto pods = standardPod(params);
    auto ms_total = params.datas.size();
    auto nodes = params.nodes;
    for(int i=0; i<ms_total; i++)
    {
        auto & pod = pods[i];
        vector<int> locs(params.datas[i].replica,0);
        for(int j=0; j<params.datas[i].replica; j++)
        {
            auto score1 = calcLeastAllocated(pod, nodes);
            auto score2 = calcBalancedAllocation(pod, nodes);
            auto score3 = calcPodTopology(nodes, locs);
            auto scoreList = {score1, score2, score3};
            auto sumScore = totalScore(scoreList);
            map<int,int> rankedScoreToLocMap;
            for(int n = 0 ;n<sumScore.size(); n++)
            {
                rankedScoreToLocMap.insert(pair<int,int>(sumScore[i], i));
            }
            for(auto it=rankedScoreToLocMap.begin();;)
            {
                int loc = it->second;
                if(nodes[loc].allocate(pod.cpu, pod.mem))
                    break;
                else if(++it == rankedScoreToLocMap.end())
                    throw("not enough capacity");
            }
        }
    }
    auto fitness = calcFitness(nodes, params.nodes);
    fstream file;
    file.open(AlgorithmFitnessTestOutputPath, ios::app);
    file<<"Kubernetes"<<" "<<fitness<<endl;
    file.close();
    file.open(AlgorithmNodeCountTestOutputPath, ios::app);
    file<<"Kubernetes"<<" "<<calcNodesCount(nodes, params.nodes)<<endl;
    file.close();
}

void testGenetic(AlgorithmParameters& params)
{
    if(!init(params.nodes, params.datas, params.totalTimeRequired, params.entrancePoint, params.bandwidth, params.rq, params.lm))
    {
        cout<<"init failed"<<endl;
        return;
    }
    auto fitnesses = iterate();
    fstream file;
    file.open(AlgorithmFitnessTestOutputPath, ios::app);
    file<<"Genetic"<<" "<<func_obj(best_chrom.Gen)<<endl;
    file.close();
    file.open(AlgorithmNodeCountTestOutputPath, ios::app);
    file<<"Genetic"<<" "<<AllocatedNodesNum(best_chrom.Gen)<<endl;
    file.close();
}

