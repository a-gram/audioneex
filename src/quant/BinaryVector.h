/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef BINARYVECTOR_H
#define BINARYVECTOR_H

#include <iostream>
#include <cstring>
#include <cassert>
#include <memory>

#if defined WIN32 || defined WIN64
  #pragma warning( disable : 4355 )
#endif


namespace Audioneex
{

class BinaryVector
{
  public:

    typedef uint8_t BitBlockType;

  private:

    class BitAccessor
    {
        BinaryVector*   m_Vector;
        mutable size_t  m_BlockIdx;
        mutable size_t  m_BitOffset;

      public:

        BitAccessor(BinaryVector* vec) :
            m_BlockIdx(0),
            m_BitOffset(0),
            m_Vector(vec)
        {}

        BitAccessor& eval_bit(size_t pos)
		{
            m_BlockIdx = pos / BITBLOCK_SIZE;
            m_BitOffset = pos % BITBLOCK_SIZE;
            return *this;
        }

        const BitAccessor& eval_bit(size_t pos) const
		{
            m_BlockIdx = pos / BITBLOCK_SIZE;
            m_BitOffset = pos % BITBLOCK_SIZE;
            return *this;
        }

        BitAccessor& operator=(const bool &val)
		{
            if(val) m_Vector->m_Data[m_BlockIdx] |= (1<<m_BitOffset);
            else    m_Vector->m_Data[m_BlockIdx] &= ~(1<<m_BitOffset);
            return *this;
        }

        BitAccessor& operator=(const int &val)
		{
            return operator=(val?true:false);
        }

        operator bool()
		{
            return (m_Vector->m_Data[m_BlockIdx] & (1<<m_BitOffset)) ? true : false;
        }

        operator bool() const
		{
            return (m_Vector->m_Data[m_BlockIdx] & (1<<m_BitOffset)) ? true : false;
        }


    };

    // Grant it access to enclosing class.
    friend class BitAccessor;

    BitBlockType* m_Data;         ///< Data buffer
    size_t        m_BlockCount;   ///< Size of data in blocks
    size_t        m_Size;         ///< Size of vector (# of binary components)
    BitAccessor   m_BitAccessor;  ///< Accessor object used to set/get bit values
    int           m_Label;        ///< Vector's label (whatever it represents)
    int           m_Distance;     ///< Distance of vector from another vector.
    bool          m_Changed;


  public:

    static const size_t BITBLOCK_SIZE = sizeof(BitBlockType)*8;


    BinaryVector() :
        m_Data(nullptr),
        m_BlockCount(0),
        m_Size(0),
        m_BitAccessor(this),
        m_Label(-1),
        m_Distance(0),
        m_Changed(false)
    {
    }


    BinaryVector(size_t size) :
        m_Data(nullptr),
        m_BlockCount(0),
        m_Size(size),
        m_BitAccessor(this),
        m_Label(-1),
        m_Distance(0),
        m_Changed(false)
    {
        assert(size > 0);

        size_t nblocks = ceil(static_cast<float>(size) /
                              static_cast<float>(BITBLOCK_SIZE));

        m_Data = new BitBlockType[nblocks];
        m_BlockCount = nblocks;
        std::memset(m_Data, 0, nblocks*sizeof(BitBlockType));
    }


    BinaryVector(const BitBlockType *data, size_t datasize, size_t size) :
        m_Data(nullptr),
        m_BlockCount(datasize),
        m_Size(size),
        m_BitAccessor(this),
        m_Label(-1),
        m_Distance(0),
        m_Changed(false)
    {
        size_t bit_buffer_size = m_BlockCount * BITBLOCK_SIZE;

        assert(size > 0 && size <= bit_buffer_size);
        assert(datasize > 0);
        assert(data);

        m_Data = new BitBlockType[datasize];
        std::copy(data, data+datasize, m_Data);

        // NOTE: To avoid wrong measures of the distance between vectors
        //       the following code clears all unused bits in the buffer
        //       (the ones beyond the vector's size) because the Hamming
        //       distance is currently performed on the whole buffer and
        //       not on the used bits only.
        for(size_t i=m_Size; i<bit_buffer_size; i++)
            m_BitAccessor.eval_bit(i) = 0;
    }


    BinaryVector(const BinaryVector &vec) :
        m_Data(nullptr),
        m_BlockCount(0),
        m_Size(0),
        m_BitAccessor(this),
        m_Label(-1),
        m_Distance(0),
        m_Changed(false)
    {
        operator=(vec);
    }


    ~BinaryVector()
    {
        delete[] m_Data;
    }


    BitAccessor& operator[](size_t n)
	{
        assert(0<=n && n<m_Size);
        return m_BitAccessor.eval_bit(n);
    }


    const BitAccessor& operator[](size_t n) const
	{
        assert(0<=n && n<m_Size);
        return m_BitAccessor.eval_bit(n);
    }


    BinaryVector& operator=(const BinaryVector &vec)
	{
        if(vec.m_Size > 0){
           // Check whether it needs reallocation
           if(m_Data==nullptr || vec.m_BlockCount != this->m_BlockCount){
              delete[] m_Data;
              m_Data = new BitBlockType[vec.m_BlockCount];
           }
           std::copy(vec.m_Data, vec.m_Data+vec.m_BlockCount, this->m_Data);
           this->m_BlockCount = vec.m_BlockCount;
           this->m_Size       = vec.m_Size;
           this->m_Label      = vec.m_Label;
           this->m_Distance   = vec.m_Distance;
           this->m_Changed    = vec.m_Changed;
        }
        return *this;
    }


    void   label(int val)              { m_Label = val; }
    int    label() const               { return m_Label; }
    void   distance(int val)           { m_Distance = val; }
    int    distance() const            { return m_Distance; }
    void   changed(bool val)           { m_Changed = val; }
    bool   changed() const             { return m_Changed; }
    size_t size() const                { return m_Size; }
    size_t bcount() const              { return m_BlockCount; }
    BitBlockType* data()               { return m_Data; }
    const BitBlockType* data() const   { return m_Data; }

};


// Binary vector comparator
inline bool operator==(const BinaryVector &vec1, const BinaryVector &vec2) 
{
    assert(vec1.size() == vec2.size());
    // TODO: It would be faster checking that all buffer blocks
    // in both vectors are equal rather than checking each
    // single bit one by one. This would require that every
    // unused bits beyond the vector size are set to 0.
    bool equal = true;
    for(size_t i=0; i<vec1.size(); i++)
        equal &= (vec1[i] == vec2[i]);
    return equal;
}

}// end namespace Audioneex

#endif
