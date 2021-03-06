#pragma once
#include "elements/swept_volume.h"
//#include "elements/swath_volume.h"

class PlannerOutput{

  public:

    uint robot_idx;

    std::vector<int> nested_idx;
    std::vector<Config> nested_q_init;
    std::vector<Config> nested_q_goal;

    //all nested robots should not be simulated, they will later be discarded
    //from ODE simulator
    std::vector<int> removable_robot_idxs;

    std::vector<int> betti_numbers_lvl0;

    std::string name_robot;
    std::string name_algorithm;

    Config q_init;
    Config q_goal;
    Config dq_init;
    Config dq_goal;

    bool success;

    Robot *robot;
    std::vector< std::vector< Config >>  paths;

  private:

    double time;
    uint nodes;

    //path
    std::vector<Config> q;
    std::vector<Config> dq;
    std::vector<Config> ddq;
    std::vector<Vector> torques;

  public:

    PlannerOutput();

    void SetTorques(std::vector<Vector> &torques_);
    const std::vector<Vector>& GetTorques();
    Config GetInitConfiguration();
    const std::vector<Config> GetKeyframes();
    void SetKeyframes(std::vector<Config> &keyframes);

};

