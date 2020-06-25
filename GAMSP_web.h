//
// Created by Administrator on 2020/6/25.
//

#ifndef GENERIC_ALGORITHM_GAMSP_WEB_H
#define GENERIC_ALGORITHM_GAMSP_WEB_H

#include <string>
#include <utility>
#include <vector>
#include "Type.h"
#include "GAMSP.h"

struct pod
{
	std::string loc;
	int cpu;
};

struct MicroserviceAllocation: SerializableJSONObject
{
	std::vector<pod> pods;
	jsonxx::Object object() override;
};

struct node
{
	std::string name;
	int milliCore;
	double mem;
	double basePrice;
	double unitPrice;
	Node convertToMipsNode() const {
		return Node{name, milliCore*MipsPerCore/1000., mem, basePrice, unitPrice};
	}
};

struct AlgorithmParameters: SerializableJSONObject
{
	std::vector<node> nodes;
	void unserialize(const std::string &) override ;
	std::vector<Node> convertToMipsNodes() const;
	explicit AlgorithmParameters(const std::string &);
	explicit AlgorithmParameters(std::vector<node> nos): nodes(std::move(nos)) {}
};

std::vector<MicroserviceAllocation> GenerateAllocations(const GAMSP&);

#endif //GENERIC_ALGORITHM_GAMSP_WEB_H
