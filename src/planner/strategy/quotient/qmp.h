#pragma once
#include "quotient_graph.h"
#include <ompl/datastructures/PDF.h>
#include <boost/graph/random.hpp> 
#include <boost/random/linear_congruential.hpp>
#include <boost/random/variate_generator.hpp>


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
    //QMP: Quotient-space roadMap Planner
    class QMP: public og::QuotientGraph{

      public:

        QMP(const ob::SpaceInformationPtr &si, Quotient *parent_);
        virtual ~QMP() override;

      protected:
        typedef boost::minstd_rand RNGType;
        RNGType rng;

        double epsilon{0.05}; //graph thickening
        double percentageSamplesOnShortestPath{0.8};
        double goalBias_{0.05};
        PDF<Vertex> vpdf;

        virtual bool SampleGraph(ob::State*) override;
        virtual ompl::PDF<og::QuotientGraph::Edge> GetEdgePDF();
        uint samplesOnShortestPath{0};

    };

  };
};
