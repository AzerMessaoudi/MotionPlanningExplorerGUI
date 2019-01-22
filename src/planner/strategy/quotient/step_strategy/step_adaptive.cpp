#include "step_adaptive.h"
using namespace ompl::geometric;
using Configuration = QuotientCover::Configuration;

bool StepStrategyAdaptive::Towards(QuotientCover::Configuration *q_from, QuotientCover::Configuration *q_to)
{
  const double distanceFromTo = quotient_cover_queue->DistanceNeighborhoodNeighborhood(q_from, q_to);
  if(distanceFromTo < 1e-10){
    return false;
  }
  std::vector<Configuration*> q_children = GenerateCandidateDirections(q_from, q_to);
  return ChooseBestDirection(q_children);
}

bool StepStrategyAdaptive::ExpandOutside(QuotientCover::Configuration *q_from)
{
  //uint NUMBER_OF_EXPANSION_SAMPLES = (quotient_cover_queue->GetQ1()->getStateDimension()+2);
  std::vector<Configuration*> q_children;
  const double radius_from = q_from->GetRadius();
  double radius_sampling = 0.1*q_from->GetRadius();

  if(verbose>0) std::cout << "Expand Outside (radius_from: " << radius_from << ")" << std::endl;

  //Step (1)
  Configuration* q_last = quotient_cover_queue->GetOutwardPointingConfiguration(q_from);
  Configuration* q_next = nullptr;

  if(!quotient_cover_queue->ComputeNeighborhood(q_last)){
    return false;
  }
  double radius_last = q_last->GetRadius();


  int ctr = 0;
  while(true){

    if(radius_last >= radius_from){
      quotient_cover_queue->AddConfigurationToCover(q_last);
      quotient_cover_queue->AddConfigurationToPriorityQueue(q_last);
      return true;
    }else{
      q_next = quotient_cover_queue->SampleOnBoundaryUniformNear(q_from, radius_sampling, q_last);
    }

    double radius_next = 0;
    if(quotient_cover_queue->ComputeNeighborhood(q_next)){
      radius_next = q_next->GetRadius();
    }

    if(radius_next > radius_last){
      q_last = q_next;
      radius_last = radius_next;
      ctr = 0;
    }else{
      ctr++;
    }
    if(ctr>3) break;
  }
  if(radius_last>0){
    quotient_cover_queue->AddConfigurationToCover(q_last);
    quotient_cover_queue->AddConfigurationToPriorityQueue(q_last);
    return true;
  }else{
    return false;
  }



  //for(uint k = 0; k < NUMBER_OF_EXPANSION_SAMPLES; k++){
  //  if(k==0){
  //    q_k = quotient_cover_queue->GetOutwardPointingConfiguration(q_from);
  //  }else{
  //    q_k = quotient_cover_queue->SampleOnBoundaryUniformNear(q_from, radius_sampling, q_k);
  //  }

  //  if(quotient_cover_queue->ComputeNeighborhood(q_k)){
  //    q_children.push_back(q_k);
  //  }else{
  //  }

  //  double radius_child = q_k->GetRadius();
  //  if(radius_child >= radius_from) return ChooseBestDirectionOnlyAddBestToQueue(q_children);



  //  if(verbose>0) std::cout << " radius:" << radius_child << std::endl;

  //  //std::cout << "from: " << radius_from << " child: " << radius_child << " ratio: " << ratio << " best: " << best_radius << std::endl;

  //  //Terminate if we found a equal or larger direction (this means we go uphill, or we go along a ridge)
  //}

  //return ChooseBestDirectionOnlyAddBestToQueue(q_children);
}

bool StepStrategyAdaptive::ChooseBestDirectionOnlyAddBestToQueue(const std::vector<Configuration*> &q_children)
{
  if(q_children.empty()){
    return false;
  }

  uint idx_best = GetLargestNeighborhoodIndex(q_children);
  Configuration *q_best = q_children.at(idx_best);
  quotient_cover_queue->AddConfigurationToCover(q_best);
  quotient_cover_queue->AddConfigurationToPriorityQueue(q_best);
  return true;
}
bool StepStrategyAdaptive::ChooseBestDirection(const std::vector<Configuration*> &q_children, bool addBestToPriorityQueue)
{
  if(q_children.empty()){
    return false;
  }

  uint idx_best = GetLargestNeighborhoodIndex(q_children);
  Configuration *q_best = q_children.at(idx_best);

  if(verbose>1) std::cout << "COVER add NBH with radius " << q_best->GetRadius() << std::endl;
  quotient_cover_queue->AddConfigurationToCover(q_best);

  //add the smaller children to the priority_configurations to be extended at
  //a later stage if required
  for(uint k = 0; k < q_children.size(); k++)
  {
    Configuration *q_k = q_children.at(k);
    if(addBestToPriorityQueue){
      quotient_cover_queue->AddConfigurationToPriorityQueue(q_k);
    }else{
      if(k!=idx_best){
        quotient_cover_queue->AddConfigurationToPriorityQueue(q_k);
      }
    }
  }
  return true;
}

uint StepStrategyAdaptive::GetLargestNeighborhoodIndexOutsideCover(const std::vector<QuotientCover::Configuration*> &q_children)
{
  double radius_best = 0;
  uint idx_best = 0;
  for(uint k = 0; k < q_children.size(); k++)
  {
    Configuration *q_k = q_children.at(k);
    double r = q_k->GetRadius();
    if(r > radius_best)
    {
      radius_best = r;
      idx_best = k;
    }
  }
  return idx_best;
}

uint StepStrategyAdaptive::GetLargestNeighborhoodIndex(const std::vector<QuotientCover::Configuration*> &q_children)
{
  double radius_best = 0;
  uint idx_best = 0;
  for(uint k = 0; k < q_children.size(); k++)
  {
    Configuration *q_k = q_children.at(k);
    double r = q_k->GetRadius();
    if(r > radius_best)
    {
      radius_best = r;
      idx_best = k;
    }
  }
  return idx_best;
}


bool StepStrategyAdaptive::ConfigurationHasNeighborhoodLargerThan(QuotientCover::Configuration *q, double radius)
{
  return (q->GetRadius() >= radius);
}

std::vector<QuotientCover::Configuration*> StepStrategyAdaptive::GenerateCandidateDirections(Configuration *q_from, Configuration *q_to)
{
  std::vector<Configuration*> q_children;

  //            \                                           |
  //             \q_k                                       |
  //              \                                         |
  // q_from        |q_proj                      q_to      |
  //              /                                         |
  //             /                                          |
  //############################################################################
  //Case(1): Projected Configuration (PC) onto neighborhood has larger radius => expand
  //directly, larger is always better
  //############################################################################
  Configuration *q_proj = new Configuration(quotient_cover_queue->GetQ1());
  Configuration *q_outside = new Configuration(quotient_cover_queue->GetQ1());
  const double radius_from = q_from->GetRadius();

  //############################################################################
  //Step1: Compute q_proj (in direction of q_to) and q_outside (in outside
  //direction relative to parent_neighbor)
  //############################################################################

  //Step1a: Project q_to onto NBH of q_from to obtain q_proj
  quotient_cover_queue->Interpolate(q_from, q_to, q_proj);
  q_proj->parent_neighbor = q_from;
  if(quotient_cover_queue->ComputeNeighborhood(q_proj)){
    q_children.push_back(q_proj);
  }

  //Step1b: Extend q_from along the line q_from->parent_neighbor-q_from to obtain
  //q_outside on the boundary of q_from
  if(q_from->parent_neighbor!=nullptr){
    const double radius_parent = q_from->parent_neighbor->GetRadius();
    double step = (radius_from+radius_parent)/(radius_parent);
    quotient_cover_queue->Interpolate(q_from->parent_neighbor, q_from, step, q_outside);
    q_outside->parent_neighbor = q_from;
    if(quotient_cover_queue->ComputeNeighborhood(q_outside)){
      q_children.push_back(q_outside);
    }
  }

  //return if non-expandable
  if(q_children.empty()) return q_children;
  //############################################################################
  //Step2: Check if any of boundary points is better than old neighborhood. In
  //that case return immediately and follow that direction
  //############################################################################

  uint k_best = 0;
  double radius_best = 0.0;
  for(uint k = 0; k < q_children.size(); k++){
    double rk = q_children.at(k)->GetRadius();
    if(rk >= radius_from){
      return q_children;
    }
    if(rk >= radius_best){
      k_best = k;
      radius_best = rk;
    }
  }
  //############################################################################
  //Step3: 
  //sampling near PC, but on boundary of q_from
  //############################################################################
  //GenerateRandomChildrenOnBoundaryAroundConfiguration(q_from, q_children.at(k_best), q_children);
  uint NUMBER_OF_EXPANSION_SAMPLES = (quotient_cover_queue->GetQ1()->getStateDimension()+2);

  Configuration *q_best = q_children.at(k_best);
  for(uint k = 0; k < NUMBER_OF_EXPANSION_SAMPLES; k++){
    q_children.push_back(quotient_cover_queue->SampleOnBoundaryUniformNear(q_from, 2*radius_best, q_best));
  }
  return q_children;
}


bool StepStrategyAdaptive::ExpandRandom(QuotientCover::Configuration *q_from)
{
  if(q_from == nullptr){
    std::cout << "Error: Expanding non-existing q" << std::endl;
    exit(0);
  }
  Configuration *q_next = new Configuration(quotient_cover_queue->GetQ1());

  //############################################################################
  double radius_from = q_from->GetRadius();
  quotient_cover_queue->GetQ1SamplerPtr()->sampleGaussian(q_next->state, q_from->state, radius_from);
  double d = quotient_cover_queue->GetQ1()->distance(q_next->state, q_from->state);
  quotient_cover_queue->GetQ1()->getStateSpace()->interpolate(q_from->state, q_next->state, radius_from/d, q_next->state);

  q_next->parent_neighbor = q_from;

  //############################################################################
  //Addconfigurationrandomperturbation
  //############################################################################
  std::vector<Configuration*> q_children;
  if(quotient_cover_queue->ComputeNeighborhood(q_next)){
    q_children.push_back(q_next);
    if(q_next->GetRadius() >= radius_from){
      //return if we go towards larger area
      return ChooseBestDirection(q_children, true);
    }
  }else{
    //no progress made
    return false;
  }
  uint NUMBER_OF_EXPANSION_SAMPLES = (quotient_cover_queue->GetQ1()->getStateDimension()+2);
  for(uint k = 0; k < NUMBER_OF_EXPANSION_SAMPLES; k++){
    q_children.push_back(quotient_cover_queue->SampleOnBoundaryUniformNear(q_from, 2*q_next->GetRadius(), q_next));
  }
  return ChooseBestDirection(q_children, true);
}
