//
// Created by Administrator on 2020/6/25.
//

#include "GAMSP_web.h"
#include <unordered_map>
using namespace jsonxx;

jsonxx::Object MicroserviceAllocation::object() {
	Object o;
	Array pos;
	for( const auto & pod : this->pods) {
		Object container;
		container << "loc" << pod.loc;
		container << "cpu" << pod.cpu;
		pos << container;
	}
	o << "pods" << pos;
	return o;
}

void AlgorithmParameters::unserialize(const std::string & json) {
	jsonxx::Object o;
	o.parse(json);
	auto nos = o.get<Array>("nodes");
	for(size_t i=0; i<nos.size(); i++) {
		auto n = nos.get<Object>(i);
		node no;
		no.name = n.get<String>("name");
		no.milliCore = n.get<Number>("milliCore");
		no.mem  = n.get<Number>("mem");
		no.basePrice = n.get<Number>("basePrice");
		no.unitPrice = n.get<Number>("unitPrice");
		this->nodes.push_back(std::move(no));
	}
}

std::vector<Node> AlgorithmParameters::convertToMipsNodes() const {
	std::vector<Node> ret;
	ret.reserve(nodes.size());
	for(const auto & no : nodes) {
		ret.push_back(no.convertToMipsNode());
	}
	return ret;
}

AlgorithmParameters::AlgorithmParameters(const std::string & json) {
	unserialize(json);
}

template<class T1, class T2>
T2 Sum(const std::unordered_map<T1,T2> & m) {
	T2 ret = 0;
	for(const auto & pair : m) {
		ret += pair.second;
	}
	return ret;
}

std::vector<MicroserviceAllocation> GenerateAllocations(const GAMSP & g) {
	std::vector<MicroserviceAllocation> ret(g.app.microservices.size(), MicroserviceAllocation());
	const auto & genes = g.bestChrom.Genes;
	for(size_t nodeIdx=0; nodeIdx<genes.size(); nodeIdx++) {
		for(const auto & pair : genes[nodeIdx].distribution) {
			auto msIdx = pair.first;
			auto total = Sum(pair.second);
			ret[msIdx].pods.push_back(pod{g.nodes[nodeIdx].name, int(g.app.microservices[msIdx].CalcResourceAllocation(total)/MipsPerCore*1000)});
		}
	}

	return ret;
}
