#include "util.h"
#include "common.h"
#include "planner/planner_input.h"
#include "cspace/cspace_input.h"
#include "planner/strategy/strategy_input.h"
#include <KrisLibrary/math/VectorTemplate.h>
#include <boost/filesystem.hpp>

//############################################################################
//PlannerMultiInput
//############################################################################
bool PlannerMultiInput::Load(const char* file){
  TiXmlDocument doc(file);
  Load(GetRootNodeFromDocument(doc));
  boost::filesystem::path p(file);

  for(uint k = 0; k < inputs.size(); k++){

    inputs.at(k)->environment_name = p.filename().string();
  }
  return (inputs.size()>0);
}

std::vector<std::string> PlannerMultiInput::GetAlgorithms(TiXmlElement *node, bool kinodynamic)
{
  TiXmlElement* node_algorithm = FindFirstSubNode(node, "algorithm");
  std::vector<std::string> algorithms;
  while(node_algorithm){
    std::string a = GetAttribute<std::string>(node_algorithm, "name");
    bool isDynamic = GetAttributeDefault<int>(node_algorithm, "dynamic", false);
    if(kinodynamic == isDynamic){
      algorithms.push_back(a);
    }else{
      std::cout << std::string(80, '*') << std::endl;
      std::cout << "Algorithm: " << a << " is " << (isDynamic?"dynamic":"geometric") <<
       " but problem is " << (kinodynamic?"dynamic":"geometric") << "." << std::endl;
      OMPL_WARN("DYNAMIC MISMATCH");
      std::cout << std::string(80, '*') << std::endl;
    }
    node_algorithm = FindNextSiblingNode(node_algorithm);
  }
  return algorithms;
}

std::vector<std::string> PlannerMultiInput::GetAlgorithmsDefault(bool kinodynamic)
{
  std::string pidef = util::GetDataFolder()+"/../settings/planner.xml";
  TiXmlDocument doc(pidef);
  TiXmlElement *node = GetRootNodeFromDocument(doc);

  CheckNodeName(node, "planner");
  return GetAlgorithms(node, kinodynamic);
}

std::vector<std::string> PlannerMultiInput::GetAlgorithmsCustom(TiXmlElement *node, bool kinodynamic)
{
  CheckNodeName(node, "plannerinput");
  return GetAlgorithms(node, kinodynamic);
}

bool PlannerMultiInput::Load(TiXmlElement *node){
  CheckNodeName(node, "world");
  TiXmlElement* node_plannerinput = FindSubNode(node, "plannerinput");

  if(!node_plannerinput){
    OMPL_WARN("XML File does not have PlannerInput");
    return false;
  }

  bool kinodynamic = GetSubNodeTextDefault<int>(node_plannerinput, "kinodynamic", false);

  TiXmlElement* node_algorithms = FindSubNode(node_plannerinput, "algorithm");
  bool hasCustomAlgorithms = (node_algorithms != nullptr);
  
  std::vector<std::string> algorithms;

  if(hasCustomAlgorithms){
    algorithms = GetAlgorithmsCustom(node_plannerinput, kinodynamic);
  }else{
    algorithms = GetAlgorithmsDefault(kinodynamic);
  }

  int i_hierarchy = CountNumberOfSubNodes(node_plannerinput, "hierarchy");

  for(uint k_algorithm = 0; k_algorithm < algorithms.size(); k_algorithm++){
    std::string name_algorithm = algorithms.at(k_algorithm);
    if(util::StartsWith(name_algorithm, "benchmark") || util::StartsWith(name_algorithm, "optimizer")){
      PlannerInput* input = new PlannerInput();
      input->name_algorithm = algorithms.at(k_algorithm);
      if(!input->Load(node_plannerinput)) return false;

      for(uint k_hierarchy = 0; k_hierarchy < (uint)i_hierarchy; k_hierarchy++){
        if(input->multiAgent){
          input->ExtractMultiHierarchy(node_plannerinput, k_hierarchy);
        }else{
          input->ExtractHierarchy(node_plannerinput, k_hierarchy);
        }
      }
      inputs.push_back(input);
    }else{
      uint number_of_hierarchies = 1;
      if(util::StartsWith(name_algorithm, "hierarchy")){
        number_of_hierarchies = i_hierarchy;
      }
      for(uint k_hierarchy = 0; k_hierarchy < number_of_hierarchies; k_hierarchy++){

        PlannerInput* input = new PlannerInput();
        input->name_algorithm = algorithms.at(k_algorithm);

        if(!input->Load(node_plannerinput)) return false;
        if(input->multiAgent){
          input->ExtractMultiHierarchy(node_plannerinput, k_hierarchy);
        }else{
          input->ExtractHierarchy(node_plannerinput, k_hierarchy);
        }

        inputs.push_back(input);
      }
    }
  }

  return true;
}

//############################################################################
//PlannerInput
//############################################################################
void PlannerInput::SetDefault()
{
  std::string pidef = util::GetDataFolder()+"/../settings/planner.xml";
  TiXmlDocument doc(pidef);
  TiXmlElement *node = GetRootNodeFromDocument(doc);

  CheckNodeName(node, "planner");

  max_planning_time = GetSubNodeText<double>(node, "maxplanningtime");
  freeFloating = GetSubNodeText<int>(node, "freeFloating");
  contactPlanner = GetSubNodeText<int>(node, "contactPlanner");
  timestep_min = GetSubNodeAttribute<double>(node, "timestep", "min");
  timestep_max = GetSubNodeAttribute<double>(node, "timestep", "max");
  max_planning_time = GetSubNodeText<double>(node, "maxplanningtime");
  epsilon_goalregion = GetSubNodeText<double>(node, "epsilongoalregion");
  pathSpeed = GetSubNodeText<double>(node, "pathSpeed");
  pathWidth = GetSubNodeText<double>(node, "pathWidth");
  pathBorderWidth = GetSubNodeText<double>(node, "pathBorderWidth");
  smoothPath = GetSubNodeText<int>(node, "smoothPath");
  kinodynamic = GetSubNodeText<int>(node, "kinodynamic");
  multiAgent = GetSubNodeText<int>(node, "multiAgent");
  name_sampler = GetSubNodeAttribute<std::string>(node, "sampler", "name");
}

bool PlannerInput::Load(TiXmlElement *node, int hierarchy_index)
{
  SetDefault();
  CheckNodeName(node, "plannerinput");

  //optional arguments

  freeFloating = GetSubNodeTextDefault(node, "freeFloating", freeFloating);
  contactPlanner = GetSubNodeTextDefault(node, "contactPlanner", contactPlanner);
  robot_idx = GetSubNodeTextDefault(node, "robot", 0);
  timestep_min = GetSubNodeAttributeDefault(node, "timestep", "min", timestep_min);
  timestep_max = GetSubNodeAttributeDefault(node, "timestep", "max", timestep_max);
  max_planning_time = GetSubNodeTextDefault(node, "maxplanningtime", max_planning_time);
  epsilon_goalregion = GetSubNodeTextDefault(node, "epsilongoalregion", epsilon_goalregion);
  pathSpeed = GetSubNodeTextDefault(node, "pathSpeed", pathSpeed);
  pathWidth = GetSubNodeTextDefault(node, "pathWidth", pathWidth);
  pathBorderWidth = GetSubNodeTextDefault(node, "pathBorderWidth", pathBorderWidth);
  smoothPath = GetSubNodeTextDefault(node, "smoothPath", smoothPath);
  name_sampler = GetSubNodeAttributeDefault<std::string>(node, "sampler", "name", name_sampler);
  kinodynamic = GetSubNodeTextDefault(node, "kinodynamic", kinodynamic);
  multiAgent = GetSubNodeTextDefault(node, "multiAgent", multiAgent);
  name_loadPath = GetSubNodeAttributeDefault<std::string>(node, "loadPath", "file", name_loadPath);
  if(multiAgent)
  {
    TiXmlElement* node_agent = FindFirstSubNode(node, "agent");
    int N = 0;
    while(node_agent!=nullptr){
      AgentInformation ai;
      ai.id = GetAttribute<int>(node_agent, "id");
      Config qzero;
      ai.q_init = GetAttribute<Config>(node_agent, "qinit");
      ai.q_goal = GetAttribute<Config>(node_agent, "qgoal");
      ai.dq_init = GetAttributeDefault<Config>(node_agent, "dqinit", qzero);
      ai.dq_goal = GetAttributeDefault<Config>(node_agent, "dqgoal", qzero);
      ai.qMin = GetAttributeDefault<Config>(node_agent, "qMin", qzero);
      ai.qMax = GetAttributeDefault<Config>(node_agent, "qMax", qzero);
      Config uzero;
      ai.uMin = GetAttributeDefault<Config>(node_agent, "uMin", uzero);
      ai.uMax = GetAttributeDefault<Config>(node_agent, "uMax", uzero);
      uMin = ai.uMin;
      uMax = ai.uMax;
      OMPL_WARN("TODO: Currently, we only support one single control input.");
      agent_information.push_back(ai);
      node_agent = FindNextSiblingNode(node_agent);
      N += ai.q_init.size();
    }
  }else{
    //necessary arguments
    q_init = GetSubNodeAttribute<Config>(node, "qinit", "config");
    q_goal = GetSubNodeAttribute<Config>(node, "qgoal", "config");

    Config dq; dq.resize(q_init.size()); dq.setZero();
    dq_init = GetSubNodeAttributeDefault<Config>(node, "dqinit", "config", dq);
    dq_goal = GetSubNodeAttributeDefault<Config>(node, "dqgoal", "config", dq);
    if(kinodynamic)
    {
      uMin = GetSubNodeAttribute<Config>(node, "control_min", "config");
      uMax = GetSubNodeAttribute<Config>(node, "control_max", "config");
    }
  }

  se3min.resize(6); se3min.setZero();
  se3max.resize(6); se3max.setZero();
  se3min = GetSubNodeAttributeDefault<Config>(node, "se3min", "config", se3min);
  se3max = GetSubNodeAttributeDefault<Config>(node, "se3max", "config", se3max);

  return true;
}

void PlannerInput::ExtractHierarchy(TiXmlElement *node, int hierarchy_index)
{
  int ctr = 0;
  TiXmlElement* node_hierarchy = FindSubNode(node, "hierarchy");
  while(ctr < hierarchy_index){
    node_hierarchy = FindNextSiblingNode(node_hierarchy);
    ctr++;
  }
  uint level = 0;
  Stratification stratification;

  if(node_hierarchy){
    TiXmlElement* lindex = FindFirstSubNode(node_hierarchy, "level");
    //layers.clear();
    while(lindex!=nullptr){
      Layer layer;

      layer.level = level++;
      layer.inner_index = GetAttribute<int>(lindex, "inner_index");
      layer.outer_index = GetAttributeDefault<int>(lindex, "outer_index", layer.inner_index);

      // layer.cspace_constant = GetAttributeDefault<double>(lindex, "cspace_constant", 0);
      layer.finite_horizon_relaxation = GetAttributeDefault<double>(lindex, "finite_horizon_relaxation", 0);
      layer.type = GetAttribute<std::string>(lindex, "type");

      stratification.layers.push_back(layer);

      lindex = FindNextSiblingNode(lindex);
    }
  }else{
    std::cout << "[WARNING] Did not specify robot hierarchy. Assuming one layer SE3RN" << std::endl;
    Layer layer;
    layer.level = 0;
    layer.inner_index = 0;
    layer.outer_index = 0;
    layer.type = "SE3RN";
    stratification.layers.push_back(layer);
  }
  stratifications.push_back(stratification);

}

bool PlannerInput::ExistsAgentAtID(int id)
{
  for(uint i = 0; i < agent_information.size(); i++){
    int idi = agent_information.at(i).id;
    if(id == idi) return true;
  }
  return false;
}

const AgentInformation& PlannerInput::GetAgentAtID(int id)
{
  for(uint i = 0; i < agent_information.size(); i++){
    int idi = agent_information.at(i).id;
    if(id == idi){
      return agent_information.at(i);
    }
  }
  std::cout << "ERROR: Could not find robot with ID " << id << std::endl;
  throw "NOT FOUND ROBOT";
}

void PlannerInput::AddConfigToConfig(Config &q, const Config &qadd, int Nclip){
  assert(qadd.size() >= Nclip);
  int N = q.size();
  q.resizePersist(N+Nclip);
  for(int k = 0; k < Nclip; k++){
    q[N+k] = qadd[k];
  }
}
void PlannerInput::AddConfigToConfig(Config &q, const Config &qadd){
  int Nadd = qadd.size();
  AddConfigToConfig(q, qadd, Nadd);
}

void PlannerInput::ExtractMultiHierarchy(TiXmlElement *node, int hierarchy_index)
{
  int ctr = 0;
  TiXmlElement* node_hierarchy = FindSubNode(node, "hierarchy");
  while(ctr < hierarchy_index){
    node_hierarchy = FindNextSiblingNode(node_hierarchy);
    ctr++;
  }
  Stratification stratification;

  if(node_hierarchy){
    TiXmlElement* lindex = FindFirstSubNode(node_hierarchy, "level");
    int lctr = 0;
    while(lindex!=nullptr){
      Layer layer;
      layer.level = lctr++;

      TiXmlElement* ri = FindFirstSubNode(lindex, "robot");
      while(ri!=nullptr){

        int id = GetAttribute<int>(ri, "id");
        layer.ids.push_back(id);

        std::string type = GetAttribute<std::string>(ri, "type");
        layer.types.push_back(type);

        layer.freeFloating.push_back(GetAttributeDefault<int>(ri, "freeFloating", 1));

        int sid = GetAttributeDefault<int>(ri, "simplification_of_id", -1);
        layer.ptr_to_next_level_ids.push_back(sid);
        layer.isMultiAgent = true;

        ri = FindNextSiblingNode(ri);
      }

      stratification.layers.push_back(layer);
      lindex = FindNextSiblingNode(lindex);
    }
    
    //############################################################################
    //compute q_init/q_goal
    //############################################################################

    q_init.clear();
    q_goal.clear();
    for(uint k = 0; k < stratification.layers.size(); k++){
      Layer &layer = stratification.layers.at(k);
      layer.q_init.clear();
      layer.q_goal.clear();
      for(uint j = 0; j < layer.ids.size(); j++){
        int idj = layer.ids.at(j);
        if(!ExistsAgentAtID(idj)) continue;
        AgentInformation agent = GetAgentAtID(idj);
        AddConfigToConfig(layer.q_init, agent.q_init);
        AddConfigToConfig(layer.q_goal, agent.q_goal);
        layer.q_inits.push_back(agent.q_init);
        layer.q_goals.push_back(agent.q_goal);
      }
    }
    q_init = stratification.layers.back().q_init;
    q_goal = stratification.layers.back().q_goal;

    //############################################################################
    //build fiber bundle projection matrix
    //############################################################################
    uint k = stratification.layers.size();
    std::vector<int> last_lvl_ids;
    std::vector<int> current_lvl_ids;
    std::vector<std::string> type;
    std::vector<int> freeFloating;
    while(k>0){
      k--;
      if(k>=stratification.layers.size()-1){
        std::sort(stratification.layers.at(k).ids.begin(), stratification.layers.at(k).ids.end());
        last_lvl_ids = stratification.layers.at(k).ids;
      }else{
        for(uint i = 0; i < last_lvl_ids.size(); i++){
          int il = last_lvl_ids.at(i);
          bool found = false;
          for(uint j = 0; j < stratification.layers.at(k).ptr_to_next_level_ids.size() ; j++){
            int ptrj = stratification.layers.at(k).ptr_to_next_level_ids.at(j);
            if(ptrj == il){
              int idj = stratification.layers.at(k).ids.at(j);
              int freeFloatingj = stratification.layers.at(k).freeFloating.at(j);
              std::string typej = stratification.layers.at(k).types.at(j);
              current_lvl_ids.push_back(idj);
              type.push_back(typej);
              freeFloating.push_back(freeFloatingj);
              found = true;
            }
          }
          if(!found){
            current_lvl_ids.push_back(-1);
            type.push_back("EMPTY_SET");
            freeFloating.push_back(false);
          }
        }
        stratification.layers.at(k).ids = current_lvl_ids;
        stratification.layers.at(k).types = type;
        stratification.layers.at(k).freeFloating = freeFloating;

        last_lvl_ids = current_lvl_ids;

        current_lvl_ids.clear();
        type.clear();
        freeFloating.clear();
      }
      std::cout << last_lvl_ids << std::endl;
      std::cout << type << std::endl;
    }
  }else{
    std::cout << "[WARNING] Did not specify robot hierarchy. Assuming one layer SE3RN" << std::endl;
    Layer layer;
    layer.level = 0;
    layer.inner_index = 0;
    layer.outer_index = 0;
    layer.type = "SE3RN";
    stratification.layers.push_back(layer);
  }
  stratifications.push_back(stratification);

}

const CSpaceInput& PlannerInput::GetCSpaceInput()
{
  cin = new CSpaceInput();
  cin->timestep_max = timestep_max;
  cin->timestep_min = timestep_min;
  cin->fixedBase = !freeFloating;
  cin->uMin = uMin;
  cin->uMax = uMax;
  cin->kinodynamic = kinodynamic;
  cin->multiAgent = multiAgent;
  return *cin;
}
const StrategyInput& PlannerInput::GetStrategyInput()
{
  sin = new StrategyInput();
  sin->q_init = q_init;
  sin->q_goal = q_goal;
  sin->dq_init = dq_init;
  sin->dq_goal = dq_goal;
  sin->name_sampler = name_sampler;
  sin->name_loadPath = name_loadPath;
  sin->name_algorithm = name_algorithm;
  sin->epsilon_goalregion = epsilon_goalregion;
  sin->max_planning_time = max_planning_time;
  sin->environment_name = environment_name;
  return *sin;
}

bool PlannerInput::Load(const char* file)
{
  TiXmlDocument doc(file);
  TiXmlElement *root = GetRootNodeFromDocument(doc);
  environment_name = file;
  return Load(root);
}
std::ostream& operator<< (std::ostream& out, const PlannerInput& pin) 
{
  out << std::string(80, '-') << std::endl;
  out << "[PlannerInput] " << pin.name_algorithm << std::endl;
  out << std::string(80, '-') << std::endl;
  out << "q_init             : " << pin.q_init << std::endl;
  out << "  dq_init          : " << pin.dq_init << std::endl;
  out << "q_goal             : " << pin.q_goal << std::endl;
  out << "  dq_goal          : " << pin.dq_goal << std::endl;
  out << "SE3_min            : " << pin.se3min << std::endl;
  out << "SE3_max            : " << pin.se3max << std::endl;
  out << "algorithm          : " << pin.name_algorithm << std::endl;
  out << "sampler            : " << pin.name_sampler << std::endl;
  out << "loadPath           : " << pin.name_loadPath << std::endl;
  out << "discr timestep     : [" << pin.timestep_min << "," << pin.timestep_max << "]" << std::endl;
  out << "max planning time  : " << pin.max_planning_time << " (seconds)" << std::endl;
  out << "epsilon_goalregion : " << pin.epsilon_goalregion << std::endl;
  out << "robot              : " << pin.robot_idx << std::endl;
  out << "environment        : " << pin.environment_name << std::endl;
  out << "stratifications    : " << pin.stratifications.size() << std::endl;
  for(uint k = 0; k < pin.stratifications.size(); k++){
    int nr_of_layers = pin.stratifications.at(k).layers.size();
    out << " - strat " << k << " has " << nr_of_layers << " layers." << std::endl;
  }
  out << std::endl;
  out << std::string(80, '-') << std::endl;
  return out;
}
