#ifndef AbstractBloomFilter_hpp
#define AbstractBloomFilter_hpp

#include <cstdbool>
#include <iostream>
#include <functional>

namespace bloom {

template <typename T>
struct HashParams_S {
    T a;        //!< Object to hash
    uint8_t b;  //!< 8-bit salt
};


template <typename T>
using HashParams = struct HashParams_S;

template <typename T>
class AbstractBloomFilter {

public:

    explicit
    AbstractBloomFilter(uint8_t numHashes, size_t numBytes)
    : m_numHashes(numHashes), m_numBytes(numBytes)
    {}

    uint8_t GetNumHashes() const {
        return m_numHashes;
    }

    size_t GetnumBytes() const {
        return m_numBytes;
    }

    virtual void Insert(T const& o) = 0;

    virtual bool Query(T const& o) const = 0;

    /** Serializes this Bloom filter into the given output stream. Output will
     *  be in a raw binary format.
     *
     *  @param os output stream to serialize the BF into
     */
    virtual void Serialize(std::ostream &os) const = 0;

//protected:
    size_t ComputeHash(T const& o, uint8_t salt) const {
        return std::hash<HashParams<T>>{}({o, salt});
    }

private:
    uint8_t m_numHashes;
    size_t m_numBytes;
    

}; // class AbstractBloomFilter

} // namespace bloom

#endif
