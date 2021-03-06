#pragma once
#include "planner/cspace/cspace_input.h"
#include "klampt.h"

#include <ompl/base/spaces/SO3StateSpace.h>
#include <ompl/base/ScopedState.h>
#include <ompl/base/StateSpace.h>
// #include <ompl/base/PlannerData.h>
// #include <ompl/base/PlannerDataGraph.h>
//#include <ompl/base/objectives/PathLengthOptimizationObjective.h>
#include <ompl/control/SpaceInformation.h>
#include <ompl/control/spaces/RealVectorControlSpace.h>
#include <Planning/RobotCSpace.h>

namespace ob = ompl::base;
namespace oc = ompl::control;

namespace ompl{
  namespace control{
    typedef std::shared_ptr<RealVectorControlSpace> RealVectorControlSpacePtr;
    typedef std::shared_ptr<StatePropagator> StatePropagatorPtr;
  }
  namespace base{
    typedef std::shared_ptr<StateValidityChecker> StateValidityCheckerPtr;
  }
}

class CSpaceOMPL
{
  friend class CSpaceOMPLMultiAgent;
  public:

    CSpaceOMPL(RobotWorld *world_, int robot_idx_);
    // CSpaceOMPL(RobotWorld *world_, std::vector<int> robot_idxs);

    void Init();
    virtual ob::SpaceInformationPtr SpaceInformationPtr();

    virtual const oc::StatePropagatorPtr StatePropagatorPtr(oc::SpaceInformationPtr si) = 0;
    virtual void ConfigToOMPLState(const Config &q, ob::State *qompl) = 0;
    virtual Config OMPLStateToConfig(const ob::State *qompl) = 0;
    virtual double GetTime(const ob::State *qompl);
    virtual bool isTimeDependent();
    virtual bool SatisfiesBounds(const ob::State*);
    virtual bool UpdateRobotConfig(Config &q);

    Config OMPLStateToConfig(const ob::ScopedState<> &qompl);
    ob::ScopedState<> ConfigToOMPLState(const Config &q);

    virtual void setStateValidityCheckerConstraintRelaxation(ob::State *xCenter, double r);
    const ob::StateValidityCheckerPtr StateValidityCheckerPtr();
    virtual Vector3 getXYZ(const ob::State*) = 0;
    virtual Vector3 getXYZ(const ob::State*, int ridx);
    virtual bool IsPlanar();

    void SetCSpaceInput(const CSpaceInput &input_);
    virtual uint GetDimensionality() const;
    virtual uint GetKlamptDimensionality() const;
    uint GetControlDimensionality() const;
    Robot* GetRobotPtr();
    std::string GetName();
    int GetRobotIndex() const;
    RobotWorld* GetWorldPtr();
    CSpaceKlampt* GetCSpaceKlamptPtr();
    const ob::StateSpacePtr SpacePtr();
    ob::StateSpacePtr SpacePtrNonConst();
    const oc::RealVectorControlSpacePtr ControlSpacePtr();

    virtual void drawConfig(const Config &q, GLDraw::GLColor color=GLDraw::GLColor(1,0,0), double scale = 1.0);

    std::vector<double> EulerXYZFromOMPLSO3StateSpace( const ob::SO3StateSpace::StateType *q );
    void OMPLSO3StateSpaceFromEulerXYZ( double x, double y, double z, ob::SO3StateSpace::StateType *q );

    virtual bool isDynamic() const = 0;
    bool isFixedBase();
    bool isFreeFloating();

    friend std::ostream& operator<< (std::ostream& out, const CSpaceOMPL& space);
    virtual void print(std::ostream& out = std::cout) const;

    void SetSufficient(const uint robot_outer_idx);
    ob::StateSpacePtr GetFirstSubspace();

    virtual bool isMultiAgent() const;

  protected:
    virtual const ob::StateValidityCheckerPtr StateValidityCheckerPtr(ob::SpaceInformationPtr si);
    virtual void initSpace() = 0;

    CSpaceInput input;

    uint Nklampt;
    uint Nompl;
    bool fixedBase{false};
    bool enableSufficiency{false};

    //klampt:
    // SE(3) x R^Nklampt
    //
    //ompl:
    // SE(3) x R^Nompl
    //
    // ompl_to_klampt: maps a dimension in R^Nompl to SE(3)xR^Nklampt
    // klampt_to_ompl: maps a dimension in SE(3)xR^Nklampt to R^Nompl
    // Note that Nompl <= Nklampt, i.e. all the zero-measure dimensions if any are collapsed
    std::vector<int> ompl_to_klampt;
    std::vector<int> klampt_to_ompl;

    ob::StateValidityCheckerPtr validity_checker;
    ob::SpaceInformationPtr si{nullptr};
    ob::StateSpacePtr space{nullptr};
    oc::RealVectorControlSpacePtr control_space{nullptr};

    Robot *robot{nullptr};
    CSpaceKlampt *klampt_cspace{nullptr};
    CSpaceKlampt *klampt_cspace_outer{nullptr};
    RobotWorld *world{nullptr};

    ////MultiAgent
    //std::vector<int> robot_idxs;
    //std::vector<Robot*> robots;
    //std::vector<CSpaceKlampt*> klampt_cspaces;

    WorldPlannerSettings worldsettings;
    int robot_idx{0};
    int robot_idx_outer{0};

};
