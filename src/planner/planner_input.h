#pragma once
#include "file_io.h"
#include <string>
#include <KrisLibrary/robotics/RobotKinematics3D.h> //Config
#include <KrisLibrary/math3d/Triangle3D.h>

struct Layer{
  int level;
  int inner_index;
  int outer_index;
  // double cspace_constant;
  double finite_horizon_relaxation;
  Config q_init;
  Config q_goal;
  std::string type;

  //multiagent
  bool isMultiAgent{false};
  int maxRobots{0};
  std::vector<int> ids;
  std::vector<Config> q_inits;
  std::vector<Config> q_goals;
  std::vector<int> ptr_to_next_level_ids;
  std::vector<std::string> types;
  std::vector<int> freeFloating;
};

struct AgentInformation{
  Config q_init;
  Config q_goal;
  Config dq_init;
  Config dq_goal;
  int id;
  Config qMin;
  Config qMax;
  Config uMin;
  Config uMax;
};

struct Stratification{
  std::vector<Layer> layers;
};

class CSpaceInput;
class StrategyInput;

class PlannerInput{
  public:
    //general input for any planner method (fixed)
    Config q_init;
    Config q_goal;
    Config dq_init;
    Config dq_goal;

    Config qMin;
    Config qMax;
    Config uMin;
    Config uMax;

    Config se3min;
    Config se3max;
    uint robot_idx;

    //multiagents
    std::vector<AgentInformation> agent_information;

    //contact-planning
    int freeFloating;
    bool contactPlanner{false};

    std::vector<Triangle3D> tris;

    std::string name_algorithm;
    std::string name_sampler;
    std::string environment_name;
    std::string name_loadPath;

    double epsilon_goalregion;
    double max_planning_time;
    double timestep_min;
    double timestep_max;

    bool smoothPath{false};
    double pathSpeed{1};
    double pathWidth{1};
    double pathBorderWidth{0.01};

    bool kinodynamic{false};
    bool multiAgent{false};

    std::vector<Stratification> stratifications;

    bool Load(const char* file);
    bool Load(TiXmlElement *node, int hierarchy = 0);
    void SetDefault();
    void ExtractHierarchy(TiXmlElement *node, int hierarchy_index);
    void ExtractMultiHierarchy(TiXmlElement *node, int hierarchy_index);
    const CSpaceInput& GetCSpaceInput();
    const StrategyInput& GetStrategyInput();

    friend std::ostream& operator<< (std::ostream& out, const PlannerInput& pin) ;
    const AgentInformation& GetAgentAtID(int id);
    void AddConfigToConfig(Config &q, const Config &qadd);
    void AddConfigToConfig(Config &q, const Config &qadd, int Nclip);
    bool ExistsAgentAtID(int id);

  private:

    CSpaceInput* cin;
    StrategyInput* sin;
};

/// @brief multiple plannerinputs (to use several algorithms inside GUI)
struct PlannerMultiInput{
  std::vector<PlannerInput*> inputs;

  bool Load(const char* file);
  bool Load(TiXmlElement *node);
  std::vector<std::string> GetAlgorithms(TiXmlElement *node, bool kinodynamic);
  std::vector<std::string> GetAlgorithmsDefault(bool kinodynamic);
  std::vector<std::string> GetAlgorithmsCustom(TiXmlElement *node, bool kinodynamic);
};

