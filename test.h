//
// Created by Administrator on 2020/5/3.
//

#ifndef GENERIC_ALGORITHM_TEST_H
#define GENERIC_ALGORITHM_TEST_H

#include"Genetic.h"

extern Chromosome best_chrom;

#define MAXNODESCORE 100

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
