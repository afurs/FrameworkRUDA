#ifndef BCschemaUtils_H
#define BCschemaUtils_H

#include <vector>
#include <string>
#include <bitset>
#include <iostream>
#include <type_traits>

#include <boost/dynamic_bitset.hpp>

namespace utilities {
  constexpr static std::size_t sNBC = 3564;
  struct SchemaBC {
    typedef uint64_t BitChunk_t;
    typedef boost::dynamic_bitset<BitChunk_t> DynBitset_t;
    typedef std::bitset<sNBC> Bitset_t;
    constexpr static std::size_t sNchunks64bit = sNBC/64 + 1 * (sNBC%64!=0) ;
    constexpr static std::size_t sNchunks64bit_padding = sNchunks64bit*64 - sNBC ;
    template<typename SchemaBC_Type>
    SchemaBC(const SchemaBC_Type &schemaBC) {
      setBCscheme(schemaBC);
      mNbunches = mBitsBC.count();
    }

    ~SchemaBC() = default;
    std::bitset<sNBC> mBitsBC{};
    DynBitset_t mBitsBC_Dyn{};
    std::vector<BitChunk_t> mVecChunks{};
    int mNbunches{};
    std::map<unsigned int, unsigned int> mMapTrains; // map first BC in train -> number of bunches in train
    std::set<unsigned int> mSetSingles; // set of single bunches
    
    template<typename SchemaBC_Type>
    void setBCscheme (const SchemaBC_Type &bcScheme) {
      if constexpr(std::is_same<SchemaBC_Type,Bitset_t >::value) {
        mBitsBC = bcScheme;
        mBitsBC_Dyn = DynBitset_t(bcScheme.to_string());
        mVecChunks.clear();
        boost::to_block_range(mBitsBC_Dyn, std::back_inserter(mVecChunks));
      }
      if else(std::is_same<SchemaBC_Type,DynBitset_t >::value) {
        mBitsBC_Dyn = bcScheme;
        if(mBitsBC_Dyn.size()>sNBC) {
          std::cout<<"\nError! Dynamic bitset size bigger than 3564: "<<mBitsBC_Dyn.size()<<"\nCutting to 3564\n";
          mBitsBC_Dyn.resize(sNBC);
        }
        else if(mBitsBC_Dyn.size()<sNBC) {
          std::cout<<"\nError! Dynamic bitset size low than 3564: "<<mBitsBC_Dyn.size()<<"\nExpanding to 3564\n";
          mBitsBC_Dyn.resize(sNBC);
        }
        std::string stBitset{};
        boost::to_string(mBitsBC_Dyn,stBitset)
        mBitsBC = Bitset_t(stBitset);
        mVecChunks.clear();
        boost::to_block_range(mBitsBC_Dyn, std::back_inserter(mVecChunks));
      }
      if else(std::is_same<SchemaBC_Type,std::vector<BitChunk_t> >::value) {
        mVecChunks = bcScheme;
        mBitsBC_Dyn = DynBitset_t(bcScheme);
        mBitsBC_Dyn.resize(sNBC);
        std::string stBitset{};
        boost::to_string(bcScheme,stBitset)
        mBitsBC = Bitset_t(stBitset);
      }
      else {
        std::cout<<"\nError! Unrecognized BC schema type!\n";
      }
      mNbunches = mBitsBC.count();
      if(mNbunches>0) {
        init();
      }
      else {
        std::cout<<"\nWarning! No bunches in BC scheme!\n";

      }
    }
    void init() {
      mMapTrains.clear();
      mSetSingles.clear();
      unsigned int nBCs{0};
      int firstBC{-1};
      for(int iBC=0;iBC<sNBC;iBC++) {
        if(mBitsBC.test(iBC)) {
          nBCs++;
          if(firstBC==-1) firstBC=iBC;
        }
        else {
          if(nBCs>1) {
            //Train BC
            mMapTrains.insert({firstBC,nBCs});
            firstBC=-1;
            nBCs=0;
          }
          else if(nBCs==1) {
            //Single BC
            mSetSingles.insert(firstBC);
            firstBC=-1;
            nBCs=0;
          }
        }
      }
    }
  };
  struct HashSchemaBC {
    
    
  };
} // namespace utilities

#endif