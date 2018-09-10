/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <cmath>
#include <cstring>
#include <fstream>

#include "common.h"
#include "Parameters.h"
#include "Codebook.h"
#include "Utils.h"
#include "audioneex.h"



void Audioneex::Codebook::FindDuplicates()
{
    int ndupes = 0;
    std::vector<BinaryVector> uwords;

    DEBUG_MSG("Checking for codebook dupes...")

    for(Cluster &clust : m_Clusters)
	{
        bool exists = false;
		
        for(BinaryVector &uw : uwords)
		{
             if(clust.Centroid == uw)
			 {
                 exists=true;
                 ndupes++;
                 break;
             }
        }
        if(!exists)
            uwords.push_back(clust.Centroid);
    }

    DEBUG_MSG(ndupes << " codeword dupes found.")
}

// ----------------------------------------------------------------------------

void Audioneex::Codebook::Analyze()
{
/*
    std::ofstream of;
    vector<float> histo;

    Gnuplot plot;
    plot.setPngPlotDevice("C:\\temp\\histo.png");

    of.open("c:\\temp\\clusters.dat");

    for(Cluster &c : m_Clusters){

        of << "ID      = " << c.ID << "\n"
           << "Npoints = " << c.Npoints << "\n"
           << "ME      = " << (c.SumD / c.Npoints) << "\n"
           << "POINTS: ";

        for(vector<pair<int,int> >::iterator it = c.Points.begin();
            it!=c.Points.end();
            ++it)

            of << "<" << (*it).first << "," << (*it).second << "> ";

        of << "\n\n";
        of.flush();

        histo.push_back(c.Npoints);
    }

    plot.plot_histogram(histo);
*/
}

// ----------------------------------------------------------------------------

/*static*/
std::unique_ptr <Audioneex::Codebook>
Audioneex::Codebook::deserialize(const uint8_t *data, size_t data_size)
{
    // Size of a centroid in bit blocks (elements)
    size_t csize = ceil(static_cast<float>(Pms::IDI_b) /
                        static_cast<float>(sizeof(BinaryVector::BitBlockType)));

    // Size of a centroid in bytes
    size_t csize_bytes = csize * sizeof(BinaryVector::BitBlockType);

    // Cluster record: | ID | SumD | Npoints | Centroid[] |
    // Total size: sizeof(int) + sizeof(float) + sizeof(int) +
    //             |C| (in bytes rounded up to the nearest bit block)
    size_t ClusterRecordSize = sizeof(uint32_t) +
	                           sizeof(float) + 
							   sizeof(uint32_t) + 
							   csize_bytes;

    if(data == nullptr || data_size == 0)
       throw Audioneex::InvalidAudioCodesException
             ("Invalid audio codes");

    // Codebook data size must be an integer multiple of the record size.
    if(data_size % ClusterRecordSize != 0)
       throw Audioneex::InvalidAudioCodesException
             ("Invalid audio codes data size");

    size_t Nwords = data_size / ClusterRecordSize;

    std::unique_ptr <Codebook> cbook (new Codebook);

    for(size_t w=0; w<Nwords; w++)
	{
        Cluster c;
        c.ID = *reinterpret_cast<const int*>(data);
        data += sizeof(uint32_t);
        c.SumD = *reinterpret_cast<const float*>(data);
        data += sizeof(float);
        c.Npoints = *reinterpret_cast<const size_t*>(data);
        data += sizeof(uint32_t);
        const BinaryVector::BitBlockType*
		pc = reinterpret_cast<const BinaryVector::BitBlockType*>(data);
        c.Centroid = BinaryVector(pc, csize, Pms::IDI);
        data += csize_bytes;
        cbook->put(c);
    }

    return cbook;
}

// ----------------------------------------------------------------------------

/*static*/
void Audioneex::Codebook::serialize(const Codebook &cbook, std::vector<uint8_t> &data)
{
    // Size of a centroid in bit blocks (elements)
    size_t csize = ceil(static_cast<float>(Pms::IDI_b) /
                        static_cast<float>(sizeof(BinaryVector::BitBlockType)));

    // Size of a centroid in bytes
    size_t csize_bytes = csize * sizeof(BinaryVector::BitBlockType);

    // Cluster record: | ID | SumD | Npoints | Centroid[] |
    // Total size: sizeof(int) + sizeof(float) + sizeof(int) +
    //             |C| (in bytes rounded up to the nearest bit block)
    size_t ClusterRecordSize = sizeof(uint32_t) + 
                               sizeof(float) + 
							   sizeof(uint32_t) + 
							   csize_bytes;

    data.reserve(cbook.size() * ClusterRecordSize);
    data.resize(0);

    // Serialize the codebook data into the byte array
    const std::vector<Cluster>& clusters = cbook.get();

    for(size_t c=0; c<clusters.size(); c++)
    {
        assert(clusters[c].ID >= 0);
        assert(clusters[c].Centroid.bcount() == csize);

        data.insert(data.end(),
                    reinterpret_cast<const uint8_t*>(&clusters[c].ID),
                    reinterpret_cast<const uint8_t*>(&clusters[c].ID) + sizeof(uint32_t));
        data.insert(data.end(),
                    reinterpret_cast<const uint8_t*>(&clusters[c].SumD),
                    reinterpret_cast<const uint8_t*>(&clusters[c].SumD) + sizeof(float));
        data.insert(data.end(),
                    reinterpret_cast<const uint8_t*>(&clusters[c].Npoints),
                    reinterpret_cast<const uint8_t*>(&clusters[c].Npoints) + sizeof(uint32_t));
        data.insert(data.end(),
                    reinterpret_cast<const uint8_t*>(clusters[c].Centroid.data()),
                    reinterpret_cast<const uint8_t*>(clusters[c].Centroid.data()) + csize);

    }

    assert(data.size() == cbook.size() * ClusterRecordSize);
}

// ----------------------------------------------------------------------------

/*static*/
void Audioneex::Codebook::Save(const Codebook &cbook, const std::string &filename)
{
    std::vector<uint8_t> ser;

    Codebook::serialize(cbook, ser);

    std::ofstream ofile(filename.c_str(), std::ios::out|std::ios::binary);

    if(!ofile)
       throw std::runtime_error
             ("Couldn't open audio codes file for writing.");

    ofile.write(reinterpret_cast<char*>(ser.data()), ser.size());

    if(ofile.fail())
        throw std::runtime_error
              ("Error while writing audio codes file.");
}

// ----------------------------------------------------------------------------

/*static*/
std::unique_ptr <Audioneex::Codebook>
Audioneex::Codebook::Load(const std::string &filename)
{
    std::ifstream ifile(filename.c_str(), std::ios::in|std::ios::binary);

    if(!ifile)
       throw std::runtime_error
             ("Couldn't open audio codes file for reading.");

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifile)),
                              (std::istreambuf_iterator<char>()));

    if(ifile.fail())
        throw std::runtime_error
              ("Error while reading audio codes file.");

    return Codebook::deserialize(data.data(), data.size());

}

// ----------------------------------------------------------------------------

Audioneex::Codebook::QResults Audioneex::Codebook::quantize(const LocalFingerprint_t &lf)
{
    assert(m_Clusters.size() > 0);
    //assert(m_Index.size() > 0);

    QResults res;
    res.word = -1;
    res.dist = -1;

    std::list<int> tlist;
    int max_sim = 0;

    for(size_t c=0; c<m_Clusters.size(); c++){

        uint32_t d = Utils::Dh(lf.D.data(), lf.D.size(),
                               m_Clusters[c].Centroid.data(),
                               m_Clusters[c].Centroid.bcount());

        int sim = Pms::IDI - d;

        assert(sim >= 0);

        if(sim > max_sim){
           max_sim = sim;
           tlist.clear();
           tlist.push_back(m_Clusters[c].ID);
        }
        else if(sim == max_sim){
           tlist.push_back(m_Clusters[c].ID);
        }
    }

    // Get best matching cluster (ideally only one but there may be ties)

    // We have only one best match
    if(tlist.size() == 1){
       res.word = *tlist.begin();
    }
    // We have ties :|
    // Ties are broken by choosing the cluster with minimum ME.
    // For this to work the clusters' MEs must be all different.
    else if(tlist.size() > 1){
       res.word = *std::max_element(tlist.begin(), tlist.end());
       //WARNING_MSG("Ties found. Chose word " << res.word)
    }
    // We have no matches (this should never happen actually)
    else{
       DEBUG_MSG("ERROR: Quantization failed. No matches found.")
    }

    // Get best match distance
    res.dist = Pms::IDI - max_sim;

    // NOTE:
    // The quantization error is clipped to fit into 1 byte.
    // The reason for this is that using 2 bytes will increase the
    // fingerprints database by about 50% due to padding, while the
    // inaccuracy due to clipping is low since most codewords will
    // have a quantization error <= 255 (tests show that about 5/1000
    // are above 255), so we prefer trading off some small accuracy
    // for some good space saving.
    res.dist = res.dist <= 255 ? res.dist : 255;

    return res;
}

