#ifndef OrdinaryBloomFilter_hpp
#define OrdinaryBloomFilter_hpp

#include <vector>
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_types.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "AbstractBloomFilter.hpp"
#include "MurmurHash.hpp"

// forward decl
namespace bloom {
    template <typename T>
    class OrdinaryBloomFilter;
}

#include "CountingBloomFilter.hpp"
#include "PairedBloomFilter.hpp"


using namespace tensorflow;

namespace std {
    template<>
    struct hash<bloom::HashParams<uint32_t>> {
        size_t operator()(bloom::HashParams<uint32_t> const &s) const {
            uint32_t out;
            bloom::MurmurHash3::murmur_hash3_x86_32((uint32_t*) &s.a, sizeof(s.a), s.b, (uint32_t*) &out);
            return out;
        }
};
}

namespace bloom {

    template <typename T>
    class OrdinaryBloomFilter : public AbstractBloomFilter<T> {

    public:

        explicit
        OrdinaryBloomFilter(uint8_t numHashes, size_t numBytes)
                : AbstractBloomFilter<T>(numHashes, numBytes) {
            m_bitarray.resize(numBytes, 0);
        }

        // Construct from bloom vector
        OrdinaryBloomFilter(uint8_t numHashes, size_t numBytes, const int8_t* ptr)
                : AbstractBloomFilter<T>(numHashes, numBytes) {
            m_bitarray.resize(numBytes);
            std::copy(ptr, ptr+numBytes, m_bitarray.begin());
        }

        virtual void Insert(T const& o) {
            unsigned int bit_pos, value, prev, byte_pos;
            for (uint8_t i = 0; i < super::GetNumHashes(); i++) {
                size_t hash = super::ComputeHash(o, i) % (super::GetnumBytes()*8);
                byte_pos = hash/8;
                bit_pos = hash%8;
                prev = m_bitarray[byte_pos];
                value = 1;
                value = value << bit_pos;
                value = value | prev;
                m_bitarray[byte_pos] = value;
            }
        }

        virtual bool Query(T const& o) const {
            unsigned int bit_pos, value, prev, byte_pos;
            for (uint8_t i = 0; i < super::GetNumHashes(); i++) {
                size_t hash = super::ComputeHash(o, i) % (super::GetnumBytes()*8);
                byte_pos = hash/8;
                bit_pos = hash%8;
                prev = m_bitarray[byte_pos];
                value = 1;
                value = value << bit_pos;
                value = value | prev;
                if (value != prev)
                    return false;
            }
            return true;
        }

        std::string Hash(T const& o) {
            std::string hash_string = "";
            std::vector<size_t> hashes;
            for (uint8_t i = 0; i < super::GetNumHashes(); i++) {
                size_t hash = super::ComputeHash(o, i) % (super::GetnumBytes()*8);
                hashes.push_back(hash);
            }
            std::sort(hashes.begin(), hashes.end());
            for (int i=0; i < hashes.size() ; i++) {
                hash_string += std::to_string(hashes[i]);
//                std::cout << "  hash: " << hash_string <<std::endl;
            }
            return hash_string;
        }

        int Get_Hash(T const& o, uint8_t i) {
                return super::ComputeHash(o, i) % (super::GetnumBytes()*8);
        }

        uint8_t Get_numHashes() {
                return super::GetNumHashes();
        }

        size_t Get_numBytes() {
                return super::GetnumBytes();
        }

        int find(const Tensor& indices, int x) {
            auto indices_flat = indices.flat<int>();
            for (int i=0; i<indices_flat.size(); ++i) {   // Dummy lookup
                if (indices_flat(i) == x)
                    return 1;
            }
        return 0;
    }

        int Compute_False_Positives(int N, const Tensor& indices) {
            int false_positives = 0;
            for (int i=0; i<N; ++i) {
                if (Query(i) && !find(indices, i)) {
                    false_positives++;
                }
            }
            return false_positives;
        }

        void fprint(FILE* f) {
            unsigned int bit_pos, byte_pos, value, byte;
            fprintf(f, "Bloom Filter: \n [ ");

            for (byte_pos=0; byte_pos<super::GetnumBytes(); byte_pos++) {
                for (bit_pos=0; bit_pos<8; bit_pos++) {
                    byte = m_bitarray[byte_pos];
                    value = 1;
                    value = value << bit_pos;
                    value = value | byte;
                    if ((value^byte) != 0)
                        fprintf(f, "0 ");
                    else fprintf(f, "1 ");
                }
            }
            fprintf(f, "]\n\n");
        }

        void print() {
            unsigned int bit_pos, byte_pos, value, byte;
            printf("Bloom Filter: \n [ ");

            for (byte_pos=0; byte_pos<super::GetnumBytes(); byte_pos++) {
                for (bit_pos=0; bit_pos<8; bit_pos++) {
                    byte = m_bitarray[byte_pos];
                    value = 1;
                    value = value << bit_pos;
                    value = value | byte;
                    if ((value^byte) != 0)
                        printf("0 ");
                    else printf("1 ");
                }
            }
            printf("]\n\n");

        }


        virtual void Serialize(std::ostream &os) const {
            uint8_t numHashes = super::GetNumHashes();
            size_t numBytes = super::GetnumBytes();

            os.write((const char *) &numHashes, sizeof(uint8_t));
            os.write((const char *) &numBytes, sizeof(size_t));

            for(size_t i = 0; i < (numBytes + 7) / 8; i++){
                uint8_t byte = 0;
                for(size_t j = 0; j < 8 && (i + j) < numBytes; j++){
                    byte = (byte << 1) | m_bitarray[8*i+j];
                }
                os.write((const char *) &byte, sizeof(uint8_t));
            }
        }

        std::vector<unsigned char>& Get_bloom() {
            return m_bitarray;
        }

        /** Create an OrdinaryBloomFilter from the content of a binary input
         * stream. No validation is performed.
         *
         * @param  is Input stream to read from
         * @return Deserialized OrdinaryBloomFilter
         */
        static OrdinaryBloomFilter<T> Deserialize(std::istream &is){
            uint8_t numHashes;
            size_t numBytes;

            is.read((char *) &numHashes, sizeof(uint8_t));
            is.read((char *) &numBytes, sizeof(size_t));

            OrdinaryBloomFilter<T> r (numHashes, numBytes);

            for(size_t i = 0; i < (numBytes + 7) / 8; i++){
                uint8_t byte;
                is.read((char *) &byte, sizeof(uint8_t));
                for(size_t j = 0; j < 8 && (i + j) < numBytes; j++){
                    r.m_bitarray[8 * i + j] = byte & (1 << (7-j));
                }
            }

            return r;
        }

        /** Halves this OrdinaryBloomFilter, reducing its size at the cost of an
         *  increased false positive ratio.
         *
         *  @return A new OrdinaryBloomFilter with half as many bits.
         */
        OrdinaryBloomFilter<T> Compress() const {
            size_t oldnumBytes = super::GetnumBytes();
            size_t newnumBytes = oldnumBytes / 2;

            OrdinaryBloomFilter<T> res(super::GetNumHashes(), newnumBytes);

            for(size_t i = 0; i < oldnumBytes; i++){
                res.m_bitarray[i % newnumBytes] = res.m_bitarray[i % newnumBytes] | m_bitarray[i];
            }

            return res;
        }

        /** Creates a PairedBloomFilter with an empty negative set, and a positive
         *  set given by this OrdinaryBloomFilter.
         *
         *  @return A new PairedBloomFilter
         */
        PairedBloomFilter<T> ToPairedBloomFilter() const {
            size_t numBytes = super::GetnumBytes();
            PairedBloomFilter<T> res(super::GetNumHashes(), numBytes);
            for(size_t i = 0; i < numBytes; i++){
                res.m_bitarray[i] = m_bitarray[i];
            }
            return res;
        }

        /** Update this Bloom filter by adding the contents of a second one.
         *  The BFs will be combined by logical OR, thus new false positives may be
         *  introduced.
         *
         *  @param other BF to combine into this one
         */
        void Union(OrdinaryBloomFilter<T> const& other){
            for(size_t i = 0; i < super::GetnumBytes(); i++){
                m_bitarray[i] = m_bitarray[i] | other.m_bitarray[i];
            }
        }

        friend OrdinaryBloomFilter<T> CountingBloomFilter<T>::ToOrdinaryBloomFilter() const;

    private:

        typedef AbstractBloomFilter<T> super;

        std::vector<unsigned char> m_bitarray;


    }; // class OrdinaryBloomFilter

} // namespace bloom

#endif
