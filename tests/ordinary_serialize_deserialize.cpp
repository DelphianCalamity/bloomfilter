#include <string>
#include <iostream>
#include <sstream>
#include "OrdinaryBloomFilter.hpp"

namespace std {
    template<> struct hash<bloom::OrdinaryBloomFilter<std::string>::HashParams> {
        size_t operator()(bloom::OrdinaryBloomFilter<std::string>::HashParams const& s) const {
            return std::hash<std::string>{}(s.a) ^ std::hash<uint8_t>{}(s.b);
        }
    };
}

int main(int argc, char *argv[]){

    std::string t1 = "Hello world!";
    std::string t2 = "foo bar baz";
    std::string t3 = "test test";
    
    bloom::OrdinaryBloomFilter<std::string> bf(4, 32);
    
    bf.Insert(t1);
    bf.Insert(t2);
    
    std::stringstream ss;
    bf.Serialize(ss);
    
    bloom::OrdinaryBloomFilter<std::string> bf_2 = bloom::OrdinaryBloomFilter<std::string>::Deserialize(ss);
    
    if(bf_2.GetNumHashes() != bf.GetNumHashes()){
        std::cout << "Error: Deserialized BF disagrees on numHashes." << std::endl;
        return 1;
    }
    
    if(bf_2.GetNumBits() != bf.GetNumBits()){
        std::cout << "Error: Deserialized BF disagrees on numBits." << std::endl;
        return 1;
    }

    if(!bf_2.Query(t1)){
        std::cout << "Error: Query for first inserted element was false." << std::endl;
        return 1;
    }
    
    if(!bf_2.Query(t2)){
        std::cout << "Error: Query for second inserted element was false." << std::endl;
        return 1;
    }
    
    if(bf_2.Query(t3)){
        std::cout << "Error: Query for non-inserted element was true." << std::endl;
        return 1;
    }
    
    std::cout << "Tests passed." << std::endl;
    
    return 0;
}
