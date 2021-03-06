#include "Network.h"

#include <ctime>
#include <chrono>
#include <fstream>
#include <iostream>

Network::Network()
    : hypostart_(0)
{
	ExtensionToIndex_[".tgf"] = 1;
	ExtensionToIndex_[".na"] = 2;
	ExtensionToIndex_[".sif"] = 3;
}

struct Comp {
    bool operator()(std::pair<unsigned int, unsigned int> p, unsigned int s) const
    { return p.first < s; }
    bool operator()(unsigned int s, std::pair<unsigned int, unsigned int> p) const
    { return s < p.first; }
    bool operator()(std::pair<unsigned int, unsigned int> p1,std::pair<unsigned int,unsigned int> p2) const
    { return p1.first < p2.first; }
  
};

unsigned int Network::getIndex(const std::string& name) const
{
	auto res = NameToIndex_.find(name);
	if(res == NameToIndex_.end()) {
		throw std::invalid_argument("Identifier not found");
	}
	return res->second;
}

const std::vector<unsigned int> Network::getParents(unsigned int id) const
{
	std::vector<unsigned int> parentIDs;
	unsigned int index = id;
	for(unsigned int row = 0; row < AdjacencyMatrix_.getRowCount(); row++) {
		if(AdjacencyMatrix_(index, row) == 1) {
			parentIDs.push_back(row);
		}
	}
	return parentIDs;
}

const std::vector<unsigned int> Network::getParents(const Node& n) const
{
	return getParents(n.getID());
}

const std::vector<unsigned int> Network::getParents(const std::string& name)
    const
{
	return getParents(getNode(name).getID());
}

std::vector<Node>& Network::getNodes() { return NodeList_; }

const std::vector<Node>& Network::getNodes() const { return NodeList_; }

void Network::cutParents(unsigned int id)
{
	getNode(id).cutParents();
}

void Network::cutParents(const std::string& name)
{
	cutParents(getNode(name).getID());
}

void Network::addEdge(unsigned int id1, unsigned int id2)
{
	AdjacencyMatrix_.setData(1, id1, id2);
	getNode(id1).setParents(getParents(id1));
}

void Network::addEdge(const std::string& name1, const std::string& name2)
{
	addEdge(getNode(name1).getID(), getNode(name2).getID());
}


void Network::removeEdge(unsigned int id1, unsigned int id2)
{
	AdjacencyMatrix_.setData(0, id1, id2);
	getNode(id1).setParents(getParents(id1));
}

void Network::removeEdge(const std::string& name1, const std::string& name2)
{
	removeEdge(getNode(name1).getID(), getNode(name2).getID());
}

Node& Network::getNode(unsigned int id) { return NodeList_[id];}//[getIndex(id)]; }

const Node& Network::getNode(const unsigned int id) const
{
	return NodeList_[id];//getIndex(id)];
}

Node& Network::getNode(const std::string& name)
{
	return NodeList_[getIndex(name)];
}

const Node& Network::getNode(const std::string& name) const
{
	return NodeList_[getIndex(name)];
}

std::ostream& operator<<(std::ostream& os, const Network& n)
{
	os << "Adjacency matrix:\n" << n.AdjacencyMatrix_ << "\n\n";
	for(const Node& node : n.NodeList_) {
		os << node << "\n";
	}
	return os;
}

void Network::readNetwork(const std::string& filename)
{
	auto idx = filename.find_last_of('.');

	if(idx == std::string::npos) {
		throw std::invalid_argument("Cannot determine file type of '" + filename + "'");
	}

	switch(ExtensionToIndex_[filename.substr(idx)]) {
		case 1:
			readTGF(filename);
			break;
		case 2:
			readNA(filename);
			break;
		case 3:
			readSIF(filename);
			break;
		default:
			throw std::invalid_argument("Unsupported file type");
	}
	assignParents();
	if (checkCycleExistence()){
		throw std::invalid_argument("The specified network contains a cycle. Thus, it can not be used.");
	}
}

void Network::readTGF(const std::string& filename)
{
	NodeList_.clear();
	AdjacencyMatrix_.clear();
	std::string line;
	std::ifstream input(filename, std::ifstream::in);
	if (! input.good()){
		throw std::invalid_argument("File not found");
		}

	std::string name;
	unsigned int index = 0;
	std::vector<std::string> names;
	while(std::getline(input, line)) {
		if(line == "#")
			break;
		std::stringstream buffer;
		buffer << line;

		size_t id1;
		buffer >> id1 >> name;
		id1=getDenseNodeIdentifier(id1);
		NodeList_.push_back(Node(0, id1, name));
		IDToIndex_[id1] = index;
		NameToIndex_[name] = index;
		std::stringstream nbuffer;
		nbuffer << id1;
		std::string ids;
		nbuffer >> ids;
		names.push_back(ids);
		index++;
	}
	AdjacencyMatrix_.resize(NodeList_.size(), NodeList_.size(), 0);
	AdjacencyMatrix_.setRowNames(names);
	AdjacencyMatrix_.setColNames(names);
	std::sort(originalIDToDense_.begin(), originalIDToDense_.end(),Comp());
	bool edges = false;
	while(std::getline(input, line)) {
		edges=true;
		std::stringstream buffer;
		buffer << line;
		size_t id1, id2;
		buffer >> id1 >> id2;
		addEdge(getNewID(id2),getNewID(id1));
	}
	if (!edges){
		throw std::invalid_argument("No edges read from file, either # is missing, or there no edges encoded");
	}
	input.close();
}

void Network::readSIF(const std::string& filename)
{
	std::string line;
	unsigned int id1;
	unsigned int id2;
	std::string relation;
	std::ifstream input(filename, std::ifstream::in);
	if (! input.good()){
		throw std::invalid_argument("File not found");
		}
	if(NodeList_.empty())
		throw std::invalid_argument(
		    "You have to read in a .na file beforehand.");
	while(std::getline(input, line)) {
		std::stringstream buffer;
		buffer << line;
		buffer >> id1 >> relation >> id2;
		if (relation == ""){
			throw std::invalid_argument("Invalid file structure of sif file");
		}
		addEdge(getNewID(id2), getNewID(id1));
		relation = "";
	}
	input.close();
}

void Network::readNA(const std::string& filename)
{
	NodeList_.clear();
	AdjacencyMatrix_.clear();
	std::string line;
	std::ifstream input(filename, std::ifstream::in);
	if (! input.good()){
		throw std::invalid_argument("File not found");
		}
	unsigned int id1;
	std::string name = "";
	std::string waste = "";
	std::string ids;
	unsigned int index = 0;
	std::vector<std::string> names;
	std::getline(input, line);
	while(std::getline(input, line)) {
		std::stringstream buffer;
		buffer << line;
		buffer >> id1 >> waste >> name;
		if (waste == "" || name == "") {
			throw std::invalid_argument("Invalid structure of na file");
		}
		id1=getDenseNodeIdentifier(id1);
		NodeList_.push_back(Node(0, id1, name));
		IDToIndex_[id1] = index;
		NameToIndex_[name] = index;
		std::stringstream nbuffer;
		nbuffer << id1;
		nbuffer >> ids;
		names.push_back(ids);
		index++;
		waste = "";
		name = "";
	}
	input.close();
	AdjacencyMatrix_.resize(NodeList_.size(), NodeList_.size(), 0);
	AdjacencyMatrix_.setRowNames(names);
	AdjacencyMatrix_.setColNames(names);
	std::sort(originalIDToDense_.begin(), originalIDToDense_.end(),Comp());
}

void Network::assignParents()
{
	for(auto& n : NodeList_) {
		n.setParents(getParents(n));
	}
}

std::unordered_map<std::string, int>& Network::getObservationsMap()
{
	return observationsMap_;
}
std::map<std::pair<int, int>, std::string>& Network::getObservationsMapR()
{
	return observationsMapR_;
}

const std::unordered_map<std::string, int>& Network::getObservationsMap() const
{
	return observationsMap_;
}
const std::map<std::pair<int, int>, std::string>& Network::getObservationsMapR()
    const
{
	return observationsMapR_;
}

void Network::setAllUnvisited()
{
	for(auto& n : NodeList_) {
		n.setUnvisited();
	}
}

void Network::performDFS(unsigned int id,
                         std::vector<unsigned int>& visitedNodes)
{
	Node& n = getNode(id);
	if(!n.isVisited()) {
		n.visit();
		visitedNodes.push_back(n.getID());
		for(auto pid : n.getParents()) {
			performDFS(pid, visitedNodes);
		}
	}
}

void Network::cycleCheck(unsigned int sourceID, unsigned int currentID, bool& result){
	if (sourceID == currentID){
		result = true;
	}
	else{
		Node& n = getNode(currentID);
		for (unsigned int pid : n.getParents()){
			cycleCheck(sourceID,pid,result);
		}
	}
}

bool Network::checkCycleExistence(unsigned int id){
	bool result = false;
	Node& n = getNode(id);
	for (int pid : n.getParents()){
		cycleCheck(id,pid,result);
	}
	return result;
}

bool Network::checkCycleExistence(){
	bool result = false;
	for (auto& node : NodeList_){
		result=checkCycleExistence(node.getID());
		if (result){
			return true;
		}
	}
	return result;
}


void Network::createBackup() { AdjacencyMatrixBackup_ = AdjacencyMatrix_; }

void Network::loadBackup()
{
	AdjacencyMatrix_ = AdjacencyMatrixBackup_;
	AdjacencyMatrixBackup_ = Matrix<unsigned int>(0, 0, 0);
}

void Network::computeFactor(Node& n) const
{
	unsigned int factor = 1;
	const auto& parents = n.getParents();
	for (int i = parents.size()-1; i>=0; i--){
		n.setFactor(factor,i);
		factor *= getNode(parents[i]).getUniqueValuesExcludingNA().size();
	}
}

int Network::reverseFactor(Node& n, unsigned int i, int row)
{
	int value = row;
	unsigned int temp = n.getRevFactor(row,i);
	if (temp==0){
		int result = -1;
		for(unsigned int j = 0; j<= i; j++) {
			int factor = n.getFactor(j);
			result = value / factor;
			value = (value % factor);
			}
		n.setRevFactor(result,row,i);
		return result;
		}
	else
		return temp;
}

bool Network::hasNode(const std::string& name) const
{
	return NameToIndex_.find(name) != NameToIndex_.end();
}

bool Network::hasValue(const std::string& nodeName,
                       const std::string& valueName) const
{
	const auto& names = getNode(nodeName).getValueNamesProb();

	return std::find(names.begin(), names.end(), valueName) != names.end();
}

size_t Network::getDenseNodeIdentifier(unsigned int originialIdentifier)
{
	size_t newID = NodeList_.size();
	originalIDToDense_.push_back(std::make_pair(originialIdentifier, newID));
	return newID;
}

unsigned int Network::getNewID(unsigned int originalIdentifier)
{
	auto low =
	    std::lower_bound(originalIDToDense_.begin(), originalIDToDense_.end(),
	                     originalIdentifier, Comp());
	if(low != originalIDToDense_.end()) {
		return low->second;
	}
	throw std::invalid_argument("Identifier not found");
}

size_t Network::size() const { return getNodes().size(); }

void Network::saveParameters() const{
	std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();
    std::time_t time_ = std::chrono::system_clock::to_time_t(time);
	std::string ts = std::ctime(&time_);
	std::ofstream file;
	std::string filename = "Parameters_"+ts;
	file.open(filename);
	for (auto& n : NodeList_){
		std::vector<unsigned int> parents = n.getParents();
		size_t rowCount=n.getNumberOfParentValues();
		size_t colCount=n.getNumberOfUniqueValuesExcludingNA();
		for (size_t row = 0; row < rowCount; row++)
			for (size_t col = 0; col < colCount; col++){
				file<<n.getName()<<"\t";
				file<<n.getProbabilityMatrix().getColNames()[col]<<"\t";
				for (auto& id : parents){
					file<<getNode(id).getName()<<"\t";
				}
				file<<n.getProbabilityMatrix().getRowNames()[row]<<"\t";
				file<<n.getProbability(col,row)<<"\n";
			}
		}
	file.close();
}

void Network::clearDynProgMatrices(){
	for (auto& n : NodeList_){
		n.clearDynProgMatrix();
	}
}

void Network::removeHypoNodes(){
	NodeList_.erase(NodeList_.begin()+hypostart_,NodeList_.end());
}

void Network::createTwinNetwork(){
	std::set<int> hypoNodes;
	for (auto& n : NodeList_){
		if (!n.getParents().empty()){
			hypoNodes.insert(n.getID());
		}	
	}
	size_t shift = NodeList_.size();
	hypostart_ = shift;
	IDMap_.resize(shift,-1);
	size_t index = shift;
	size_t newID = shift;
	for (auto id : hypoNodes){
		IDMap_[id]=newID;
		newID++;
	}

	for (auto id : hypoNodes){
		Node hypoNode = getNode(id);
		hypoNode.setName(hypoNode.getName()+"*");
		hypoNode.setID(IDMap_[id]);
		std::vector<unsigned int> newParents;
		for (auto& p : getNode(id).getParents()){
			if (hypoNodes.find(p)!=hypoNodes.end()){
				newParents.push_back(IDMap_[p]);
			}
			else{
				newParents.push_back(p);
			}
		}
		hypoNode.setParents(newParents);
		IDToIndex_[hypoNode.getID()] = index;
		NameToIndex_[hypoNode.getName()] = index;
		index++;	
		NodeList_.push_back(hypoNode);
	}
}

unsigned int Network::getHypoStart(){
	return hypostart_;
}

unsigned int Network::getHypoID(unsigned int originalID) const{
	return IDMap_[originalID];
}

void Network::reset(){
	for (auto& node : NodeList_){
		node.reset();
	}
}
