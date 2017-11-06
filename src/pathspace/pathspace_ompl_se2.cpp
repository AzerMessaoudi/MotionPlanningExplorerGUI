#include "pathspace_ompl_se2.h"
#include "pathspace_atomic.h"
#include "elements/roadmap_decorator.h"

#include "planner/cspace/cspace_factory.h"
#include "planner/strategy/strategy_geometric.h"
#include "gui/drawMotionPlanner.h"
#include "gui/colors.h"

PathSpaceOMPLSE2::PathSpaceOMPLSE2(RobotWorld *world_, PathSpaceInput* input_):
  PathSpaceOMPL(world_, input_)
{
}

std::vector<PathSpace*> PathSpaceOMPLSE2::Decompose(){
  WorldPlannerSettings worldsettings;
  worldsettings.InitializeDefault(*world);

  CSpaceFactory factory(input->GetCSpaceInput());

  int robot_idx = input->robot_idx;
  Robot *robot = world->robots[robot_idx];
  SingleRobotCSpace* kcspace = new SingleRobotCSpace(*world,robot_idx,&worldsettings);
  CSpaceOMPL *cspace = factory.MakeGeometricCSpaceSE2(robot, kcspace);
  cspace->print();

  StrategyGeometric strategy;
  StrategyOutput output(cspace);
  StrategyInput strategy_input = input->GetStrategyInput();
  strategy_input.cspace = cspace;
  strategy.plan(strategy_input, output);

  std::vector<PathSpace*> decomposedspace;
  decomposedspace.push_back( new PathSpaceAtomic(world, input->GetNextLayer()) );
  RoadmapPtr roadmap1 = output.GetRoadmapPtr();
  RoadmapDecoratorSE2Ptr roadmap( new RoadmapDecoratorSE2(roadmap1) );
  decomposedspace.at(0)->SetRoadmap( roadmap );

  if(output.hasExactSolution()){
    roadmap->SetShortestPath( output.GetShortestPath() );
    decomposedspace.at(0)->SetShortestPath( output.GetShortestPath() );
  }
  return decomposedspace;
}

void PathSpaceOMPLSE2::DrawGL(GUIState&){
  uint ridx = input->robot_idx;
  Robot* robot = world->robots[ridx];
  const Config qi = input->q_init;
  const Config qg = input->q_goal;

  GLDraw::drawRobotAtConfig(robot, qi, lightGreen);
  GLDraw::drawRobotAtConfig(robot, qg, lightRed);
}

