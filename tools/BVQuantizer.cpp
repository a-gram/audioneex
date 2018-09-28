/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <cmath>
#include <boost/unordered_set.hpp>

#ifdef WIN32
 #include <tbb42/tbb/blocked_range.h>
 #include <tbb42/tbb/parallel_for.h>
#else
 #include <tbb42/blocked_range.h>
 #include <tbb42/parallel_for.h>
#endif

#include "common.h"
#include "BVQuantizer.h"
#include "Utils.h"



namespace Audioneex
{

BVQuantizer::BVQuantizer(int K) :
    m_K(K)
{
}

// ----------------------------------------------------------------------------

void BVQuantizer::addPoint(BinaryVector &point)
{
    assert( !m_Points.empty() ? point.size() == m_Points[m_Points.size()-1].size() : 1);
    m_Points.push_back(point);
}

// ----------------------------------------------------------------------------

void BVQuantizer::RandomSeeding()
{
    DEBUG_MSG( "Random seeding ..." )

    Utils::rng::natural<int> rndNumber(0, m_Points.size());

    boost::unordered::unordered_set<int> points;

    // Randomly select K points from data set
    while(m_Clusters.size() < m_K)
	{
        int h = rndNumber();
		
        if(points.find(h) == points.end())
		{
           points.insert(h);
           Cluster clust;
           clust.Centroid = m_Points[h];
           m_Clusters.push_back(clust);
        }
    }

    assert(m_Clusters.size() == m_K);
}

// ----------------------------------------------------------------------------

void BVQuantizer::KmeansPP()
{
    DEBUG_MSG("k-means++ seeding ...")

    Utils::rng::natural<int> int_rnd(0, m_Points.size());
    Utils::rng::real<float> real_rnd(0.f, 1.f);

    // Choose initial centroid at random from data points
    int c0 = int_rnd();

    Cluster clust;
    clust.ID = 0;
    clust.Centroid = m_Points[c0];

    m_Clusters.push_back(clust);

    std::vector<float> p(m_Points.size(), std::numeric_limits<float>::max());

    // Compute remaining centroids

    for(int cj=1; cj<m_K; cj++)
    {
        float psum = 0.0f,
              cum = 0.0f,
              d;

        // Update data points' p.d.f.

        for(size_t h=0; h<m_Points.size(); h++) 
		{
            d = Utils::Dh(m_Points[h], m_Clusters[cj-1].Centroid);
            p[h] = d<p[h] ? d : p[h];
            psum += p[h];
        }

        // Randomly sample a point using the updated p.d.f. by
        // Inverse Transform Sampling

        float u = real_rnd();

        for(size_t v=0; v<p.size(); v++)
		{
            if( (cum += p[v] / psum) > u )
			{
                clust.ID = cj;
                clust.Centroid = m_Points[v];
                m_Clusters.push_back(clust);
                break;
            }
		}

        DEBUG_MSG("c[" << cj <<"] ")
    }

    assert(m_Clusters.size() == m_K);
}

// ----------------------------------------------------------------------------

// Function object executed by the parallel threads.

class PointsClusterer_t 
{
 public:
 
    std::vector<BinaryVector>*  Points;
    std::vector<Cluster>*       Clusters;

    void operator()(tbb::blocked_range<size_t> &r) const {

        for(size_t i=r.begin(); i!=r.end(); ++i)
        {
            int d = Utils::Dh(Points->at(i), Clusters->at(0).Centroid);
            int cj = 0;
            int dist = 0;

            Points->at(i).changed(false);

            for(size_t j=1; j<Clusters->size(); j++)
                if( (dist = Utils::Dh(Points->at(i), Clusters->at(j).Centroid)) < d ){
                   d = dist;
                   cj = j;
                }

            // Check whether vector's cluster changed and signal it
            if(Points->at(i).label() != cj){
               Points->at(i).label(cj);
               Points->at(i).changed(true);
            }

            Points->at(i).distance(d);
        }
    }
};


std::shared_ptr <Codebook> BVQuantizer::Kmedians()
{

    assert(m_K > 0);
    assert(m_Points.size() > m_K);

    m_Clusters.clear();

    DEBUG_MSG("Creating " << m_K << " clusters from " << m_Points.size() << " data points..")

    float  clusters_change = 0.0f;  // clusters change ratio
    float  clusters_ME = 0.0f;      // clusters Mean Error
    int    c  = -1;
    int    it = 0;
    size_t Nx = m_Points[0].size();

    // Create data structures
    std::vector<std::vector< array_2D > >  BitCounter(m_K, std::vector<array_2D>(Nx));

    std::shared_ptr <Codebook> codebook(new Codebook);

    // k-means++ seeding
    KmeansPP();
    //RandomSeeding();

    // Create the context used by the function object.

    PointsClusterer_t clusterPoints;
    clusterPoints.Points   = &m_Points;
    clusterPoints.Clusters = &m_Clusters;

    // k-medians

    DEBUG_MSG("k-medians...")

    while(1)
    {
        // Clusterize points around current centroids
        tbb::parallel_for(tbb::blocked_range<size_t>(0,
                                                     m_Points.size(),
                                                     m_Points.size()/4),
                          clusterPoints);

        // Update clusters
        for(size_t i=0; i<m_Points.size(); i++)
        {
            c = m_Points[i].label();

            m_Clusters[c].Npoints++;
            m_Clusters[c].SumD += m_Points[i].distance();
            m_Clusters[c].Points.push_back(std::pair<int,int>(m_Points[i].FID, m_Points[i].LID));
			
            for(size_t x=0; x<Nx; x++)
                m_Points[i][x] ? BitCounter[c][x][1]++
                               : BitCounter[c][x][0]++;

            clusters_change += m_Points[i].changed() ? 1 : 0;
            clusters_ME += m_Points[i].distance();
        }

        // Compute clusters change ratio and mean error
        clusters_change = (clusters_change / m_Points.size()) * 100;
        clusters_ME /= m_Points.size();

        DEBUG_MSG("Cluster change: " << clusters_change << "%\t"
                 << "ME:" << clusters_ME << "\t It.: " << it)

        // Reset clusters and compute new centroids if they're not yet stable,
        // stop clustering otherwise.
        if(clusters_change > 1 && it<=30)
        {
           for(size_t j=0; j<m_Clusters.size(); j++) 
		   {
               m_Clusters[j].Npoints = 0;
               m_Clusters[j].SumD = 0;
               m_Clusters[j].Points.clear();
			   
               for(size_t x=0; x<Nx; x++)
			   {
                   if(BitCounter[j][x][0] > BitCounter[j][x][1])
                      m_Clusters[j].Centroid[x] = 0;
                   if(BitCounter[j][x][0] < BitCounter[j][x][1])
                      m_Clusters[j].Centroid[x] = 1;

                   // Reset counter
                   BitCounter[j][x][0] = 0;
                   BitCounter[j][x][1] = 0;
               }
           }
        }
        else
           break;

        clusters_change = 0;
        clusters_ME = 0;
        it++;
    }

    // Set the codebook
    codebook->set(m_Clusters);

    return codebook;

}

}// end namespace Audioneex

