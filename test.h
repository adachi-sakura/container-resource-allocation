//
// Created by Administrator on 2020/5/3.
//

#ifndef GENERIC_ALGORITHM_TEST_H
#define GENERIC_ALGORITHM_TEST_H

#include"Genetic.h"

extern Chromosome best_chrom;

#define MAXNODESCORE 100

const std::string GenerationTestOutputPath = "output/maxgenes.csv";
const std::string MutationTestOutputPath = "output/pmutation.csv";
const std::string CrossOverTestOutputPath = "output/pxover.csv";
const std::string EliteTestOutputPath = "output/elite.csv";
const std::string PenaltyTestOutputPath = "output/penalty.csv";
const std::string AlgorithmFitnessTestOutputPath = "output/fitness.csv";
const std::string AlgorithmNodeCountTestOutputPath = "output/nodeCount.csv";


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
AlgorithmParameters getDefaultParameters();

#endif //GENERIC_ALGORITHM_TEST_H
