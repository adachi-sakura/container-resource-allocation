//
// Created by Administrator on 2020/6/11.
//

#ifndef GENERIC_ALGORITHM_GAMSP_H
#define GENERIC_ALGORITHM_GAMSP_H

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <ctime>
#include <functional>

#define POPSIZE  200             //个体数

#define MAXGENES  100            //最大迭代次数

#define PXOVER  0.94            //交叉概率

#define PMUTATION 0.05          //变异概率

#define ELITERATE 0.2

#define COSTTOTAL 1000000

const double MipsPerCore = 2500;

using MicroserviceType = int;
using UserRequestType = int;
using ImageType = int;
struct Gene
{
	using Distribution = std::unordered_map<MicroserviceType , std::unordered_map<UserRequestType , int>>;
	Distribution distribution; //节点请求量分布
	std::unordered_set<ImageType> library;
	std::unordered_map<MicroserviceType , int>  amounts() const;
	int GetSpecificAmount(MicroserviceType, UserRequestType) const;
};

struct Chromosome
{
	std::vector<Gene> Genes;
	double fitness=0;
	double rfitness=0;
	double cfitness=0;
	Chromosome() = default;
	explicit Chromosome(size_t size):Genes(size) {}
};

struct Node
{
	std::string name;
	double mips;
	double mem;
	double basePrice;
	double unitPrice;
};

struct Image
{
	double mem;
};


struct Microservice
{
	double resource;
	std::unordered_set<ImageType> imageDependencies;
	int requestSum;
	Microservice() = default;
	Microservice(double, int, const std::vector<ImageType>&);
	double CalcResourceAllocation(int) const;
};

struct Application
{
	std::vector<Microservice> microservices;
	int requestTypeNum = 0;
	using User2Ms = std::vector<std::unordered_map<MicroserviceType , int>>;
	User2Ms user2msNum;
	using Ms2User = std::vector<std::unordered_map<UserRequestType , int>>;
	Ms2User ms2userNum;
	MicroserviceType distancedMsIdx(UserRequestType, int) const;
	UserRequestType distancedUserIdx(MicroserviceType, int) const;
	Application() = default;
	Application(const std::vector<double> &, const std::vector<int> &, const std::vector<std::vector<ImageType>> &, const std::vector<std::vector<int>> &);
};

struct Random
{
	std::default_random_engine engine;
	std::uniform_real_distribution<double> random_probability;
	Random(): engine(time(nullptr)), random_probability(0., 1.){}
	double GenerateProbability() {
		return random_probability(engine);
	}
};

class GAMSP
{
public:
	std::vector<Chromosome> popCurrent;
	std::vector<Chromosome> popNext;
	Chromosome bestChrom;
	std::vector<Node> nodes;
	std::vector<Image> images;
	Application app;
	mutable Random rand;
	GAMSP(): popCurrent(POPSIZE), popNext(POPSIZE) {}
	GAMSP(std::vector<Node>& ns, std::vector<Image>& is, Application & ap)
	:popCurrent(POPSIZE, Chromosome(ns.size())), popNext(POPSIZE, Chromosome(ns.size())),
	nodes(ns), images(is), app(ap) {}
	GAMSP(std::vector<Node>&& ns, std::vector<Image>&& is, Application && ap)
	:popCurrent(POPSIZE, Chromosome(ns.size())), popNext(POPSIZE, Chromosome(ns.size())),
	nodes(std::move(ns)), images(std::move(is)), app(std::move(ap)) {}
	bool restrain(const std::vector<Gene> &) const;
	double MipsAllocation(const Gene &) const;
	double MemAllocation(const Gene &) const;
	double Cost(const std::vector<Gene> &) const;
	void init();
	void eval();
	void elite();
	void select();
	void crossover();
	void externalCrossOver();
	void internalCrossOver();
	void mutate();
	void migrate();
	void Run();
	void mutateGene(Gene &);
	using ServicePairs = std::vector<std::pair<UserRequestType , MicroserviceType >>;
	ServicePairs RandomServicePairs() const;
	struct utilization
	{
		size_t nodeNum;
		double mips;
		double allocation;
		double calcUtilization() const {
			return allocation/mips;
		}
		bool operator<(const utilization & b) const
		{
			return this->calcUtilization() < b.calcUtilization();
		}
		bool isAvailable(double newAllocation) {
			return allocation+newAllocation <= mips;
		}
	};
	std::vector<utilization> RankedUtilization(std::vector<Gene> &, const std::function<bool(const utilization&, const utilization&)> & = nullptr);
	void tryMigrate(size_t, std::vector<utilization> &, std::vector<Gene> &);
	void tryFineTuneMigrate(size_t, std::vector<utilization> &, std::vector<Gene> &);
	void migrateBetweenTwo(Gene&, utilization&, Gene&, utilization&);
	void fineTuneMigrateBetweenTwo(Gene&, utilization&, Gene&, utilization&);
	void initGenes(std::vector<Gene> &);
	void initLibrary(Gene &, const Node &);
	std::vector<Gene*> availableNodes(std::vector<Gene> &, const Microservice&) const;
	void checkGeneDistributions(const std::vector<Gene> &) const;
};

void test();

#endif //GENERIC_ALGORITHM_GAMSP_H
