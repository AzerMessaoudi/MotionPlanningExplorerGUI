#include "common.h"
#include "rrt_unidirectional.h"
#include "elements/plannerdata_vertex_annotated.h"
#include "planner/cspace/validitychecker/validity_checker_ompl.h"

using namespace ompl::geometric;

RRTUnidirectional::RRTUnidirectional(const base::SpaceInformationPtr &si, Quotient *parent ): og::Quotient(si, parent)
{
  deltaCoverPenetration_ = 0.05;
  goalBias_ = 0.05;
  totalNumberOfSamples = 0;
  graphLength = 0.0;
  epsilon = 0.005;
}

RRTUnidirectional::~RRTUnidirectional(void)
{
}
double RRTUnidirectional::Distance(ob::State *s_lhs, ob::State *s_rhs){
  double d = si_->distance(s_lhs, s_rhs);
  return d;
}

void RRTUnidirectional::Sample(Configuration *q_random){
  totalNumberOfSamples++;
  if(parent == nullptr){
    if(!hasSolution && rng_.uniform01() < goalBias_){
      goal->sampleGoal(q_random->state);
    }else{
      Q1_sampler->sampleUniform(q_random->state);
    }
  }else{
    if(!hasSolution && rng_.uniform01() < goalBias_){
      goal->sampleGoal(q_random->state);
    }else{
      ob::SpaceInformationPtr Q0 = parent->getSpaceInformation();
      base::State *s_X1 = X1->allocState();
      base::State *s_Q0 = Q0->allocState();

      X1_sampler->sampleUniform(s_X1);
      parent->SampleGraph(s_Q0);
      mergeStates(s_Q0, s_X1, q_random->state);

      X1->freeState(s_X1);
      Q0->freeState(s_Q0);
    }
  }
}

void RRTUnidirectional::setRange(double distance)
{
  maxDistance_ = distance;
}

double RRTUnidirectional::getRange() const
{
  return maxDistance_;
}

uint RRTUnidirectional::GetNumberOfVertices() const
{
  return G_->size();
}

uint RRTUnidirectional::GetNumberOfEdges() const
{
  std::vector<Configuration *> configs;
  if (G_){
    G_->list(configs);
  }
  uint ctr_edges = 0;
  for (auto &config : configs)
  {
    if (config->parent != nullptr)
    {
      ctr_edges++;
    }
  }
  return ctr_edges;
}


RRTUnidirectional::Configuration* RRTUnidirectional::Nearest(Configuration *q_random)
{
  return G_->nearest(q_random);
}

RRTUnidirectional::Configuration* RRTUnidirectional::Connect(Configuration *q_near, Configuration *q_random)
{
  //two strategies: either check directly if the edge between q_near and
  //q_random is feasible and add that (as implemented by RRTConnect)
  //or: move as long towards q_random as the edge is feasible. Once it becomes
  //infeasible, set q_random to the last feasible vertex. Might be more
  //efficient, but why is that not implemented in RRTConnect?
  //=> it seems we often need to reject samples, especially if the boundary box
  //of the workspace is not tight. 

  double d = Distance(q_near->state, q_random->state);

  //##############################################################################
  // move towards q_random until infeasible
  //##############################################################################
  double d_segment = si_->getStateSpace()->getLongestValidSegmentFraction();
  //uint nd = si_->getStateSpace()->validSegmentCount(q_near->state, q_random->state);

  double d_cum = 0;
  double d_lastvalid = 0;
  ob::State *q_test = si_->allocState();
  while(d_cum <= d)
  {
    si_->getStateSpace()->interpolate(q_near->state, q_random->state, d_cum / d, q_test);
    if(si_->isValid(q_test)){
      d_lastvalid = d_cum;
    }else{
      break;
    }
    d_cum += d_segment;
  }
  si_->freeState(q_test);

  if(d_lastvalid<=0) return nullptr;

  auto *q_new = new Configuration(si_);
  q_new->state = si_->allocState();

  si_->getStateSpace()->interpolate(q_near->state, q_random->state, d_lastvalid / d, q_new->state);

  //si_->copyState(q_new->state, q_random->state);
  q_new->parent = q_near;
  q_new->parent_edge_weight = Distance(q_near->state, q_new->state);
  graphLength += q_new->parent_edge_weight;

  G_->add(q_new);

  return q_new;
  // //##############################################################################
  // // q_new_state <- BALL_maxDistance_(q_near) in direction of q_random
  // //##############################################################################
  // double maxD = maxDistance_;
  // if(d > maxD){
  //   //maxD/d -> t \in [0,1] on boundary
  //   si_->getStateSpace()->interpolate(q_near->state, q_random->state, maxD / d, q_random->state);
  //   d = maxD; //new distance between q_near and q_random
  // }


  // //##############################################################################
  // // extend the tree from q_near towards q_new
  // //##############################################################################

  // if(si_->checkMotion(q_near->state, q_random->state)){
  //   auto *q_new = new Configuration(si_);
  //   si_->copyState(q_new->state, q_random->state);
  //   q_new->parent = q_near;
  //   q_new->parent_edge_weight = si_->distance(q_near->state, q_new->state);
  //   graphLength += q_new->parent_edge_weight;// + q_new->parent_edge_weight*epsilon*si_->getSpaceMeasure();

  //   auto checkerPtr = static_pointer_cast<OMPLValidityChecker>(si_->getStateValidityChecker());
  //   double d1 = checkerPtr->Distance(q_new->state);
  //   q_new->openset = new cover::OpenSetHypersphere(si_, q_new->state, d1);

  //   G_->add(q_new);

  //   return q_new;
  // }
  // return nullptr;
}

bool RRTUnidirectional::ConnectedToGoal(Configuration* q)
{
  double dist=0.0;
  if(q != nullptr){
    if(goal->isSatisfied(q->state, &dist)){
      hasSolution = true;
      return true;
    }
  }
  return false;
}

void RRTUnidirectional::ConstructSolution(Configuration *configuration_goal)
{
  if (configuration_goal != nullptr){

    std::vector<Configuration *> q_path;
    while (configuration_goal != nullptr){
      q_path.push_back(configuration_goal);
      configuration_goal = configuration_goal->parent;
    }

    auto path(std::make_shared<PathGeometric>(si_));
    for (int i = q_path.size() - 1; i >= 0; --i){
      path->append(q_path[i]->state);
    }
    pdef_->addSolutionPath(path);
  }
}

void RRTUnidirectional::Init()
{

  checkValidity();
  goal = dynamic_cast<ob::GoalSampleableRegion *>(pdef_->getGoal().get());

  while (const ob::State *st = pis_.nextStart()){
    q_start = new Configuration(si_);
    si_->copyState(q_start->state, st);

    G_->add(q_start);
  }

  //if (const ob::State *st = pis_.nextGoal()){
  //}else{
  //  OMPL_ERROR("%s: There is no valid goal state!", getName().c_str());
  //  exit(0);
  //}

  if (G_->size() == 0){
    OMPL_ERROR("%s: There are no valid initial states!", getName().c_str());
    exit(0);
  }
}


void RRTUnidirectional::clear()
{
  Planner::clear();
  freeMemory();
  if(G_){
    G_->clear();
  }
  hasSolution = false;
  lastExtendedConfiguration = nullptr;
  q_start = nullptr;
  q_goal = nullptr;

  pis_.restart();
}

void RRTUnidirectional::freeMemory()
{
  if (G_)
  {
    std::vector<Configuration *> configurations;
    G_->list(configurations);
    for (auto &configuration : configurations)
    {
      if (configuration->state != nullptr)
      {
        si_->freeState(configuration->state);
      }
      delete configuration;
    }
  }
}

void RRTUnidirectional::setup(void)
{
  Planner::setup();
  tools::SelfConfig sc(si_, getName());
  sc.configurePlannerRange(maxDistance_);

  if (!G_){
    G_.reset(tools::SelfConfig::getDefaultNearestNeighbors<Configuration *>(this));
  }
  G_->setDistanceFunction([this](const Configuration *a, const Configuration *b)
                           {
                           //distance between surrounding hyperspheres. if
                           //overlapping, return 0
                              //double ra = a->GetRadius();
                              //double rb = b->GetRadius();
                              //return max( si_->distance(a->state, b->state)-ra-rb, 0.0);
                              return si_->distance(a->state, b->state);
                           });

}

void RRTUnidirectional::getPlannerData(base::PlannerData &data) const
{
  std::vector<Configuration *> vertices;

  if (G_){
    G_->list(vertices);
  }

  if (lastExtendedConfiguration != nullptr){
    data.addGoalVertex(PlannerDataVertexAnnotated(lastExtendedConfiguration->state, 0, lastExtendedConfiguration->GetRadius()));
  }

  for (auto &vertex : vertices)
  {
    double d = vertex->GetRadius();
    if (vertex->parent == nullptr){
      data.addStartVertex(PlannerDataVertexAnnotated(vertex->state, 0, d));
    }else{
      double dp = vertex->parent->GetRadius();
      data.addEdge(PlannerDataVertexAnnotated(vertex->parent->state, 0, dp), PlannerDataVertexAnnotated(vertex->state, 0, d));
    }
    if(!vertex->state){
      std::cout << "vertex state does not exists" << std::endl;
      si_->printState(vertex->state);
      exit(0);
    }
  }
}

void RRTUnidirectional::Grow(double t)
{
  Configuration *q_random = new Configuration(si_);

  Sample(q_random);

  Configuration *q_near = Nearest(q_random);
  Configuration *q_new = Connect(q_near, q_random);

  if( parent!=nullptr && q_new != nullptr){
    if(static_cast<og::RRTUnidirectional*>(parent)->lastSampled!=nullptr){
      static_cast<og::RRTUnidirectional*>(parent)->lastSampled->successfulSamples++;
    }
  }

  if(!hasSolution && q_new != nullptr){
    lastExtendedConfiguration = q_new;
  }
  if(q_random->state != nullptr){
    si_->freeState(q_random->state);
  }
  delete q_random;
}


bool RRTUnidirectional::HasSolution()
{
  if(!hasSolution){
    if(ConnectedToGoal(lastExtendedConfiguration)){
      hasSolution = true;
    }
  }
  return hasSolution;
}

void RRTUnidirectional::CheckForSolution(ob::PathPtr &solution)
{
  if(!hasSolution) return;

  Configuration *configuration_goal = lastExtendedConfiguration;
  if (configuration_goal != nullptr){

    std::vector<Configuration *> q_path;
    while (configuration_goal != nullptr){
      q_path.push_back(configuration_goal);
      configuration_goal = configuration_goal->parent;
    }

    auto path(std::make_shared<PathGeometric>(si_));
    for (int i = q_path.size() - 1; i >= 0; --i){
      path->append(q_path[i]->state);
    }
    solution = path;
    goalBias_ = 0.0;
  }
}
bool RRTUnidirectional::SampleGraph(ob::State *q_random_graph)
{
  PDF<Configuration*> pdf = GetConfigurationPDF();

  Configuration *q = pdf.sample(rng_.uniform01());
  double t = rng_.uniform01();
  const ob::State *q_from = q->state;
  const ob::State *q_to = q->parent->state;
  Q1->getStateSpace()->interpolate(q_from, q_to, t, q_random_graph);
  Q1_sampler->sampleGaussian(q_random_graph, q_random_graph, epsilon);

  lastSampled = q;
  lastSampled->totalSamples++;
  return true;
}
ompl::PDF<RRTUnidirectional::Configuration*> RRTUnidirectional::GetConfigurationPDF()
{
  PDF<Configuration*> pdf;
  std::vector<Configuration *> configurations;
  double t = rng_.uniform01();
  if(t<0.2){
    if(G_){
      G_->list(configurations);
    }
    for (auto &configuration : configurations)
    {
      if(!(configuration->parent == nullptr))
      {
        //double d = exp(-configuration->GetRadius());
        pdf.add(configuration, configuration->parent_edge_weight);
      }
    }
  }else{
    //shortest path heuristic
    Configuration *configuration = lastExtendedConfiguration;
    std::vector<Configuration *> q_path;
    if (configuration != nullptr){
      while (configuration != nullptr){
        if(configuration->parent !=nullptr){
          q_path.push_back(configuration);
        }

        configuration = configuration->parent;
      }
    }

    //std::cout << successfulSamplesPerSlide << std::endl;
    //std::vector<double> weights;
    for(uint k = 0; k < q_path.size(); k++){
      //double dsp = si_->distance(q_path.at(k)->state, q_path.at(k)->parent->state);
      Configuration *configuration = q_path.at(k);
      //uint N = configuration->successfulSamples;
      double w = configuration->parent_edge_weight;
      pdf.add(configuration, w);
      //std::cout << std::string(80, '-') << std::endl;
      //std::cout << "edge " << k << ": length: "  << dsp << " weight: " << w << std::endl;
      //si_->printState(q_path.at(k)->state);
      //si_->printState(q_path.at(k)->parent->state);
      //weights.push_back(d);
      //pdf.add(configuration, 1.0);//configuration->parent_edge_weight);
    }
    //exit(0);
    //std::cout << weights << std::endl;
    //std::cout << std::string(80, '-') << std::endl;
  }

  return pdf;
}
