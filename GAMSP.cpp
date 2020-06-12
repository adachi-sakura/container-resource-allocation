//
// Created by Administrator on 2020/6/11.
//
#include "GAMSP.h"
#include <map>
#include <cassert>
#include <iostream>
#include <algorithm>
using namespace std;

bool GAMSP::restrain(const std::vector<Gene> & genes) const
{
	for(size_t i=0; i<genes.size(); i++)
	{
		if(MipsAllocation(genes[i]) > nodes[i].mips)
			return false;
	}

	for(size_t msIdx=0; msIdx<app.ms2userNum.size(); msIdx++)
	{
		for(const auto & pair : app.ms2userNum[msIdx])
		{
			UserRequestType userReqIdx = pair.first;
			int num = pair.second;
			int sum = 0;
			for(const auto & gene : genes)
			{
				sum += gene.GetSpecificAmount(msIdx, userReqIdx);
			}
			if(sum != num)
				return false;
		}
	}

	//todo refactor
	for(size_t i=0; i<genes.size(); i++)
	{
		if(MemAllocation(genes[i]) > nodes[i].mem)
			return false;
	}


	return true;
}

double GAMSP::MipsAllocation(const Gene & gene) const {
	unordered_map<MicroserviceType , int> msAmounts = gene.amounts();
	double sumAllocation = 0;
	for(const auto & pair : msAmounts)
	{
		MicroserviceType msIdx = pair.first;
		int reqAmount = pair.second;
		sumAllocation += app.microservices[msIdx].CalcResourceAllocation(reqAmount);
	}
	return sumAllocation;
}

double GAMSP::MemAllocation(const Gene & gene) const {
	double sum = 0;
	for( int imageIdx : gene.library)
	{
		sum += images[imageIdx].mem;
	}
	return sum;
}

template <class T>
bool DeepEqual(const unordered_set<T> & s1, const unordered_set<T> & s2) {
	if(s1.size() != s2.size())
		return false;
	for(const auto & item : s1)
	{
		if(s2.find(item) == s2.end())
			return false;
	}
	return true;
}

template <class T>
bool IsSubset(const unordered_set<T> & s1, const unordered_set<T> & s2) {
	if(s1.size() > s2.size())
		return false;
	for(const auto & item : s1)
	{
		if(s2.find(item) == s2.end())
			return false;
	}
	return true;
}

vector<int> randomAmounts(size_t count, unsigned int range) {
	uniform_int_distribution<int> randomRange(0, range);
	default_random_engine engine(time(nullptr));
	vector<int> ret;
	for(size_t i=0; i<count; i++)
		ret.push_back(randomRange(engine));
	return ret;
}

vector<int> randomDistributions(size_t count, unsigned int total) {
	if(count == 0)
		return vector<int>();
	auto amounts = randomAmounts(count-1, total);
	sort(amounts.begin(), amounts.end());
	amounts.push_back(total-amounts.back());
	for(size_t i=1; i<amounts.size(); i++)
		amounts[i] -= amounts[i-1];
	return amounts;
}

double GAMSP::Cost(const std::vector<Gene> & genes) const {
	double cost = 0;

	for(size_t i=0; i<genes.size(); i++)
	{
		//todo refactor
		double sumAllocation = 0;
		auto amounts = genes[i].amounts();
		for(const auto & pair : amounts)
		{
			MicroserviceType msIdx = pair.first;
			if(!IsSubset(app.microservices[msIdx].imageDependencies, genes[i].library))
				continue;
			int msReqNum = pair.second;
			sumAllocation += app.microservices[msIdx].CalcResourceAllocation(msReqNum);
		}
		cost += nodes[i].basePrice + nodes[i].unitPrice*sumAllocation/1000;
	}
	return cost;
}

void GAMSP::eval() {
	for(auto & chromosome : popCurrent)
	{
		double aim = Cost(chromosome.Genes);
		if(restrain(chromosome.Genes))
			chromosome.fitness = COSTTOTAL-aim;
		else
			chromosome.fitness = COSTTOTAL-aim*100;
	}
}

void GAMSP::elite() {
	size_t elite_pos=-1;
	multimap<double, int> fitness_rank; // fitness,position 按fitness自小到大排列的map
	for(size_t i=0; i<popCurrent.size(); i++)
	{
		if(restrain(popCurrent[i].Genes))
		{
			if (elite_pos == -1)
				elite_pos = i;
			else
				elite_pos = popCurrent[i].fitness>popCurrent[elite_pos].fitness ? i : elite_pos;
		}
		fitness_rank.insert(make_pair(popCurrent[i].fitness, i));
	}
	if (elite_pos != -1)
		bestChrom = popCurrent[elite_pos];
	if (elite_pos == -1)
		cout<<"bad chromosome!"<<endl;
	auto it = fitness_rank.begin();
	for(int i=0; i<ELITERATE*POPSIZE; i++,it++)
	{
		popCurrent[it->second] = bestChrom;
	}
}

void GAMSP::select() {
	double fitness_sum = 0;
	for(auto & chromsome : popCurrent)
		fitness_sum += chromsome.fitness;
	double cfitness = 0;
	for(auto & chromsome : popCurrent)
	{
		chromsome.rfitness = chromsome.fitness/fitness_sum;
		cfitness += chromsome.rfitness;
		chromsome.cfitness = cfitness;
	}
	for(auto & empty_chromsome : popNext)
	{
		double roulette = rand.GenerateProbability();
		for(const auto& chromsome : popCurrent)
		{
			if (roulette < chromsome.cfitness)
			{
				empty_chromsome = chromsome;
				break;
			}
		}
	}
	popCurrent = popNext;
}

template <class T>
void RangeSwap(vector<T> & v1, vector<T> & v2, int num) {
	assert(v1.size() == v2.size());
	assert(num <= v1.size());
	vector<T> temp;
	copy_n(v1.begin(), num, back_inserter(temp));
	copy_n(v2.begin(), num, v1.begin());
	copy_n(temp.begin(), num, v2.begin());
}

void GAMSP::externalCrossOver() {
	uniform_int_distribution<int> randomPop(0, POPSIZE-1);
	uniform_int_distribution<int> randomNode(0, nodes.size());
	for(int cnt=0; cnt<POPSIZE; cnt++)
	{
		if(rand.GenerateProbability() >= PXOVER)
			continue;
		auto & chrom1 = popCurrent[randomPop(rand.engine)];
		auto & chrom2 = popCurrent[randomPop(rand.engine)];
		int xNum = randomNode(rand.engine);
		RangeSwap(chrom1.Genes, chrom2.Genes, xNum);
	}
}

void GAMSP::internalCrossOver() {
	uniform_int_distribution<int> randomNode(0, nodes.size()-1);
	for(auto & chromosome : popCurrent)
	{
		if(rand.GenerateProbability() >= PXOVER)
			continue;
		int nodeIdx1 = randomNode(rand.engine);
		int nodeIdx2 = randomNode(rand.engine);
		while(MipsAllocation(chromosome.Genes[nodeIdx1]) > nodes[nodeIdx2].mips ||
				MipsAllocation(chromosome.Genes[nodeIdx2]) > nodes[nodeIdx1].mips)
		{
			nodeIdx1 = randomNode(rand.engine);
			nodeIdx2 = randomNode(rand.engine);
		}
		swap(chromosome.Genes[nodeIdx1], chromosome.Genes[nodeIdx2]);
	}
}

void GAMSP::mutate() {
	for(auto & chromosome : popCurrent)
	{
		for(auto & gene : chromosome.Genes)
		{
			if(rand.GenerateProbability() >= PMUTATION)
				continue;
			mutateGene(gene);
		}
	}
}

GAMSP::ServicePairs GAMSP::RandomServicePairs() const {
	//todo refactor
	ServicePairs ret;
	for(int userReqIdx=0; userReqIdx<app.requestTypeNum; userReqIdx++)
	{
		uniform_int_distribution<int> randomMsIdx(0, app.user2msNum[userReqIdx].size()-1);
		int r = randomMsIdx(rand.engine);
		ret.push_back(make_pair(userReqIdx, app.distancedMsIdx(userReqIdx, r)));
	}
	return ret;
}

void GAMSP::migrate() {
	for(auto & chrom : popCurrent)
	{
		auto sortedUtilizations = RankedUtilization(chrom.Genes);
		for(size_t i=0; i<sortedUtilizations.size(); i++)
			tryMigrate(i, sortedUtilizations, chrom.Genes);
	}
}

void GAMSP::mutateGene(Gene & gene) {
	ServicePairs pairs = RandomServicePairs();
	for(const auto & pair : pairs)
	{
		UserRequestType userReqIdx = pair.first;
		MicroserviceType msIdx = pair.second;
		uniform_int_distribution<int> randomAmount(0, app.user2msNum[userReqIdx][msIdx]);
		gene.distribution[msIdx][userReqIdx] = randomAmount(rand.engine);
	}
}

std::vector<GAMSP::utilization> GAMSP::RankedUtilization(std::vector<Gene> & genes) {
	vector<utilization> uts;

	for(size_t nodeIdx=0; nodeIdx<genes.size(); nodeIdx++)
	{
		auto allocation = MipsAllocation(genes[nodeIdx]);
		if(allocation == 0)
			continue;
		uts.push_back(utilization{
			.nodeNum = nodeIdx,
			.mips = nodes[nodeIdx].mips,
			.allocation = allocation,
		});
	}
	sort(uts.begin(), uts.end());
	return uts;
}

void GAMSP::tryMigrate(size_t idx, vector<utilization> & sortedUts, std::vector<Gene> & genes) {

	auto sourceNodeIdx = sortedUts[idx].nodeNum;
	auto & sourceGene = genes[sourceNodeIdx];
	for(size_t targetIdx=sortedUts.size()-1; targetIdx>idx; targetIdx--)
	{
		auto targetNodeIdx = sortedUts[targetIdx].nodeNum;
		auto & targetGene = genes[targetNodeIdx];
		migrateBetweenTwo(sourceGene, sortedUts[idx], targetGene, sortedUts[targetIdx]);
	}
}

void GAMSP::migrateBetweenTwo(Gene & sourceGene, GAMSP::utilization & sourceUt, Gene & targetGene, GAMSP::utilization & targetUt) {
	//todo refactor
	vector<pair<MicroserviceType , UserRequestType >> eraseAfterMigration;
	for(const auto  & msPair : sourceGene.distribution)
	{
		auto msIdx = msPair.first;
		if(!IsSubset(app.microservices[msIdx].imageDependencies, targetGene.library))
			continue;
		for(const auto & reqPair : msPair.second)
		{
			auto usReqIdx = reqPair.first;
			auto amount = reqPair.second;
			auto newAllocation = app.microservices[msIdx].CalcResourceAllocation(amount);
			if(!targetUt.isAvailable(newAllocation))
				continue;

			targetUt.allocation += newAllocation;
			targetGene.distribution[msIdx][usReqIdx] += amount;
			sourceUt.allocation -= newAllocation;
			eraseAfterMigration.emplace_back(msIdx, usReqIdx);
		}
	}
	for(const auto & pair : eraseAfterMigration)
	{
		auto msIdx = pair.first;
		auto usReqIdx = pair.second;
		sourceGene.distribution[msIdx].erase(usReqIdx);
		if(sourceGene.distribution[msIdx].empty())
			sourceGene.distribution.erase(msIdx);
	}
}

void GAMSP::init() {
	for(auto & chrom : popCurrent)
	{
		do{
			initGenes(chrom.Genes);
		}while(!restrain(chrom.Genes));
	}
}

void GAMSP::initGenes(vector<Gene> & genes) {
	for(size_t i=0; i<genes.size(); i++)
		initLibrary(genes[i], nodes[i]);

	for(size_t msIdx=0; msIdx<app.microservices.size(); msIdx++)
	{
		auto nodePtrs = availableNodes(genes, app.microservices[msIdx]);
		for(size_t usReqIdx=0; usReqIdx<app.ms2userNum[msIdx].size(); usReqIdx++)
		{
			int reqNum = app.ms2userNum[msIdx][usReqIdx];
			auto distributions = randomDistributions(nodePtrs.size(), reqNum);
			assert(distributions.size() == nodePtrs.size());
			for(size_t i=0; i<nodePtrs.size(); i++)
				nodePtrs[i]->distribution[msIdx][usReqIdx] = distributions[i];
		}
	}
}

template <class T>
const T& NthItem(unordered_set<T> & s, size_t idx) {
	assert(idx < s.size());
	auto it = s.begin();
	for(size_t i=0; i<idx; i++, it++)
	{}
	return *it;
}

void GAMSP::initLibrary(Gene & gene, const Node & node) {
	unordered_set<ImageType> imgSet;
	double mem = node.mem;
	for(size_t i=0; i<images.size(); i++)
		imgSet.insert(i);
	while(!imgSet.empty())
	{
		uniform_int_distribution<int> randomSetIdx(0, imgSet.size()-1);
		ImageType imgIdx = NthItem(imgSet, randomSetIdx(rand.engine));
		if(mem > images[imgIdx].mem)
		{
			gene.library.insert(imgIdx);
			mem -= images[imgIdx].mem;
		}
		imgSet.erase(imgIdx);
	}
}

std::vector<Gene*> GAMSP::availableNodes(std::vector<Gene> & genes, const Microservice & ms) const {
	vector<Gene*> ret;
	for(auto & gene : genes)
	{
		if(!IsSubset(ms.imageDependencies, gene.library))
			ret.push_back(&gene);
	}
	return ret;
}



unordered_map<MicroserviceType , int> Gene::amounts() const
{
	unordered_map<MicroserviceType , int> ret;
	for(const auto & i : distribution)
	{
		MicroserviceType msIdx = i.first;
		int sum = 0;
		for(const auto & j : i.second)
		{
			sum += j.second;
		}
		ret[msIdx] = sum;
	}

	return ret;
}

int Gene::GetSpecificAmount(MicroserviceType msIdx, UserRequestType userReqIdx) const {
	auto distributionIt = distribution.find(msIdx);
	if(distributionIt == distribution.end())
		return 0;
	auto & m = distributionIt->second;
	auto mIt = m.find(userReqIdx);
	if(mIt == m.end())
		return 0;

	return mIt->second;
}

double Microservice::CalcResourceAllocation(int amount) const {
	return amount*resource/requestSum;
}

Microservice::Microservice(double r, int req, vector<ImageType> & v): resource(r), requestSum(req) {
	for(size_t i=0; i<v.size(); i++)
	{
		if(v[i])
			imageDependencies.insert(i);
	}
}

MicroserviceType Application::distancedMsIdx(UserRequestType idx1, int offset) const {
	assert(idx1 < user2msNum.size());
	auto & ht = user2msNum[idx1];

	assert(offset < ht.size());
	auto it = ht.begin();
	for(int i=0; i<offset; i++,it++)
	{}
	return it->first;
}

UserRequestType Application::distancedUserIdx(MicroserviceType idx1, int offset) const {
	assert(idx1 < ms2userNum.size());
	auto & ht = ms2userNum[idx1];

	assert(offset < ht.size());
	auto it = ht.begin();
	for(int i=0; i<offset; i++,it++)
	{}
	return it->first;
}

Application::Application(std::vector<double> & vRes, std::vector<int> & vReq, std::vector<std::vector<ImageType>> & vImgs,
                         std::vector<std::vector<int>> & req2ms) {
	assert(vRes.size() == vReq.size());
	assert(vRes.size() == vImgs.size());
	for(size_t i=0; i<vRes.size(); i++)
	{
		microservices.emplace_back(vRes[i], vReq[i], vImgs[i]);
	}
	for(size_t reqIdx=0; reqIdx<req2ms.size(); reqIdx++)
	{
		for(size_t msIdx=0; msIdx<req2ms[reqIdx].size(); msIdx++)
		{
			int num = req2ms[reqIdx][msIdx];
			user2msNum[reqIdx][msIdx] = num;
			ms2userNum[msIdx][reqIdx] = num;
		}
	}

}
