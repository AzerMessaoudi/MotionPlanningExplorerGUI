#pragma once

#include <tinyxml.h>
#include "util.h"
#include "gui.h"
#include "info.h"
#include "planner/planner_input.h"

class EnvironmentLoader{
  private:
    std::string file_name;

    std::string name_environment;
    std::string name_robot;
    std::string name;

    RobotWorld world;
    ForceFieldBackendPtr _backend;
    Info info;
    PlannerInput pin;

  public:
    RobotWorld& GetWorld();
    RobotWorld* GetWorldPtr();
    Robot* GetRobotPtr();
    ForceFieldBackendPtr GetBackendPtr();
    PlannerInput GetPlannerInput();

    EnvironmentLoader(const char *xml_file);

    bool LoadPath(const char* file);
    bool LoadPath(TiXmlElement *node);
    std::vector<Config> GetKeyframesFromFile(const char* file);

};
