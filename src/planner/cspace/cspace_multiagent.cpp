#include "gui/colors.h"
#include "planner/cspace/cspace_multiagent.h"
#include "planner/cspace/validitychecker/validity_checker_multiagent.h"
#include <ompl/base/spaces/RealVectorStateSpace.h>

#include <boost/foreach.hpp>
#include <numeric>

CSpaceOMPLMultiAgent::CSpaceOMPLMultiAgent(std::vector<CSpaceOMPL*> cspaces):
  CSpaceOMPL(cspaces.front()->GetWorldPtr(), cspaces.front()->GetRobotIndex()), cspaces_(cspaces)
{
}

bool CSpaceOMPLMultiAgent::isDynamic() const
{
  for(uint k = 0; k < cspaces_.size(); k++){
    CSpaceOMPL *ck = cspaces_.at(k);
    if(ck->isDynamic()) return true;
  }
  return false;
}

void CSpaceOMPLMultiAgent::drawConfig(const Config &q, GLColor color, double scale){
  std::vector<Config> qks = splitConfig(q);
  for(uint k = 0; k < cspaces_.size(); k++){
    CSpaceOMPL *ck = cspaces_.at(k);
    ck->drawConfig(qks.at(k), color, scale);
  }
}

const oc::StatePropagatorPtr CSpaceOMPLMultiAgent::StatePropagatorPtr(oc::SpaceInformationPtr si)
{
  OMPL_ERROR("MultiAgentCspace has no StatePropagatorPtr");
  throw "No StatePropagatorPtr.";
}

void CSpaceOMPLMultiAgent::initSpace()
{
  for(uint k = 0; k < cspaces_.size(); k++){
    CSpaceOMPL *ck = cspaces_.at(k);
    ck->initSpace();
    space = space + ck->SpacePtr();
    Nklampts.push_back( ck->GetKlamptDimensionality() );
    Nompls.push_back( ck->GetDimensionality() );
  }
  Nompl = std::accumulate(Nompls.begin(), Nompls.end(), 0);
  Nklampt = std::accumulate(Nklampts.begin(), Nklampts.end(), 0);

  if(space->isCompound()){
    subspaceCount = space->as<ob::CompoundStateSpace>()->getSubspaceCount();
    if(subspaceCount != (int)cspaces_.size()){
      OMPL_ERROR("Mismatch");
      throw "Mismatch";
    }
  }else{
    OMPL_ERROR("Not compound state space");
    throw "Not compound state space";
  }

}

uint CSpaceOMPLMultiAgent::GetKlamptDimensionality() const{
  std::cout << "Klampt DIM: " << Nklampt << std::endl;
  return Nklampt;
}

void CSpaceOMPLMultiAgent::print(std::ostream& out) const
{
  for(uint k = 0; k < cspaces_.size(); k++){
    CSpaceOMPL *ck = cspaces_.at(k);
    ck->print();
  }
}

std::vector<Config> CSpaceOMPLMultiAgent::splitConfig(const Config &q)
{
  int ctr = 0;
  std::vector<Config> qks;
  for(uint k = 0; k < Nklampts.size(); k++){
    int Nk = Nklampts.at(k);
    Config qk; qk.resize(Nk);
    for(int j = 0; j < Nk; j++){
      qk[j] = q[j+ctr];
    }
    qks.push_back(qk);
    ctr+=Nk;
  }
  return qks;
}
void CSpaceOMPLMultiAgent::ConfigToOMPLState(const Config &q, ob::State *qompl)
{
  int ctr = 0;
  std::vector<Config> qks = splitConfig(q);
  for(uint k = 0; k < Nklampts.size(); k++){
    int Nk = Nklampts.at(k);
    Config qk; qk.resize(Nk);
    for(int j = 0; j < Nk; j++){
      qk[j] = q[j+ctr];
    }
    qks.push_back(qk);
    ctr+=Nk;
  }

  for(int k = 0; k < subspaceCount; k++){
    // space->as<ob::CompoundStateSpace>()->getSubspace(k);
    ob::RealVectorStateSpace::StateType* qkOMPL = qompl->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(k);
    cspaces_.at(k)->ConfigToOMPLState(qks.at(k), qkOMPL);
  }
  // std::cout << q << std::endl;
  // si->printState(qompl);
}
Config CSpaceOMPLMultiAgent::OMPLStateToConfig(const ob::State *qompl)
{

  int ctr = 0;
  Config q; q.resize(Nklampt);
  for(int k = 0; k < subspaceCount; k++){
    // space->as<ob::CompoundStateSpace>()->getSubspace(k);
    const ob::RealVectorStateSpace::StateType* qkOMPL = qompl->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(k);
    Config qk = cspaces_.at(k)->OMPLStateToConfig(qkOMPL);
    for(int j = 0; j < qk.size(); j++){
      q[j+ctr] = qk[j];
    }
    ctr += qk.size();
  }
  // std::cout << q << std::endl;
  // si->printState(qompl);
  return q;
}

const ob::StateValidityCheckerPtr CSpaceOMPLMultiAgent::StateValidityCheckerPtr(ob::SpaceInformationPtr si)
{
  validity_checker = std::make_shared<OMPLValidityCheckerMultiAgent>(si, cspaces_);
  return validity_checker;
}