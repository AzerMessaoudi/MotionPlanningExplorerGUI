#include <stdio.h>
#include <ctime>
#include <KrisLibrary/GLdraw/GL.h>
#include <KrisLibrary/GLdraw/drawextra.h>

#include <Contact/Grasp.h> //class Grasp
#include <Planning/ContactCSpace.h>
#include <Modeling/MultiPath.h>
#include <Planning/PlannerSettings.h>
#include <KrisLibrary/planning/AnyMotionPlanner.h>
#include <Modeling/DynamicPath.h>
#include <Modeling/Paths.h>
#include <Control/PathController.h>

#include "util.h"
#include "info.h"
#include "controller.h"
#include "gui.h"
#include "planner/planner_ompl_irreducible.h"
#include "plannersetup/plannersetup_sentinel_pipe_irreducible.h"

int main(int argc,const char** argv) {
  RobotWorld world;
  Info info;
  ForceFieldBackend backend(&world);
  //SimTestBackend backend(&world);
  WorldSimulation& sim=backend.sim;

  //############################################################################

  PlannerSetupSentinelPipeIrreducible setup(&world);
  setup.LoadAndInitSim(backend);
  Config p_init = setup.GetInitialConfig();
  Config p_goal = setup.GetGoalConfig();

  //############################################################################
  //free space planner
  //############################################################################

  MotionPlannerOMPLIrreducible planner(&world, &sim);

  if(planner.solve(p_init, p_goal)){
    std::vector<Config> keyframes = planner.GetKeyframes();
    backend.AddPath(keyframes);
  }

  //backend.VisualizeStartGoal(p_init, p_goal);
  backend.VisualizePlannerTree(planner.GetTree());
  backend.Save("sentinel_pipe.xml");
  //backend.Load("kinodynamic_solution_tunnel_environment.xml");

  ////############################################################################
  ////guification
  ////############################################################################

  std::cout << "start GUI" << std::endl;
  GLUIForceFieldGUI gui(&backend,&world);
  gui.SetWindowTitle("SweptVolumePath");
  gui.Run();

  return 0;
}



