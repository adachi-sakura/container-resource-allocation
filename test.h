//
// Created by Administrator on 2020/5/3.
//

#ifndef GENERIC_ALGORITHM_TEST_H
#define GENERIC_ALGORITHM_TEST_H

#include"Genetic.h"
#include<unordered_map>

extern Chromosome best_chrom;

#define MAXNODESCORE 100

const std::string GenerationTestOutputPath = "output/maxgenes.csv";
const std::string MutationTestOutputPath = "output/pmutation.csv";
const std::string CrossOverTestOutputPath = "output/pxover.csv";
const std::string EliteTestOutputPath = "output/elite.csv";
const std::string PenaltyTestOutputPath = "output/penalty.csv";
const std::string AlgorithmFitnessTestOutputPath = "output/fitness.csv";
const std::string AlgorithmNodeCountTestOutputPath = "output/nodeCount.csv";
const std::string AlgorithmCpuUsageTestOutputPath = "output/cpuUsage.csv";
using str2strMap = const std::unordered_map<std::string, std::string>;
str2strMap AlgorithmPathMap = {
		{"AlgorithmFitnessTestOutputPath", "output/fitness.csv"},
		{"AlgorithmNodeCountTestOutputPath", "output/nodeCount.csv"},
		{"AlgorithmCpuUsageTestOutputPath", "output/cpuUsage.csv"},
};

struct Pod
{
    double cpu;
    double mem;
};

void testIteration(AlgorithmParameters&);
void testMutation(AlgorithmParameters&);
void testCrossOver(AlgorithmParameters&);
void testElite(AlgorithmParameters&);
void testPenalty(AlgorithmParameters&);
void testRoundRobin(AlgorithmParameters&);
void testRandom(AlgorithmParameters&);
void testKubernetes(AlgorithmParameters&);
void testGenetic(AlgorithmParameters&);
void ClearAlgorithmFiles();
AlgorithmParameters getDefaultParameters();

#endif //GENERIC_ALGORITHM_TEST_H
