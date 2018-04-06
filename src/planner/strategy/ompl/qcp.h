#pragma once
#include "prm_basic.h"
#include "planner/cover/cover.h"
#include "planner/cover/cover_convex_partition.h"
#include <ompl/datastructures/PDF.h>

namespace ob = ompl::base;
namespace og = ompl::geometric;


namespace ompl
{
  namespace base
  {
      OMPL_CLASS_FORWARD(OptimizationObjective);
  }
  namespace geometric
  {
    //QSCP: Quotient-space simplicial Complex Planner
    //SC : simplicial complex
    //
    class QCP: public og::Quotient{

      public:

        QCP(const ob::SpaceInformationPtr &si, Quotient *previous_);
        virtual ~QCP() override;

        virtual void Init() override;
        virtual void Grow(double t) override;
        virtual void CheckForSolution(ob::PathPtr &solution) override;
        virtual void getPlannerData(ob::PlannerData &data) const override;

      protected:
        std::vector<const ob::State*> startS_;
        std::vector<const ob::State*> goalS_;
        cover::CoverConvexPartition cspace_cover;
        ompl::RNG rng;
    };

  };
};
