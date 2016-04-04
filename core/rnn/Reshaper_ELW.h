#ifndef SIBYL_RESHAPER_ELW_H_
#define SIBYL_RESHAPER_ELW_H_

#include <array>

#include "Reshaper.h"

namespace sibyl
{

class ItemMem_ELW : public ItemMem
{
public:
    std::array<PQ, szTb> lastTb;
    FLOAT initKospi200;
    FLOAT initThp;
    ItemMem_ELW() : ItemMem(), lastTb{}, initKospi200(0.0f), initThp(0.0f) {}
};

class Reshaper_ELW : public Reshaper<ItemMem_ELW>
{
public:
    Reshaper_ELW(unsigned long maxGTck_,
                 TradeDataSet *pTradeDataSet_,
                 std::vector<std::string> *pFileList_,
                 const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*));

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state);
    
private:
    FLOAT ReshapePrice(FLOAT p) override { return (FLOAT) (std::log((FLOAT) 1 + p) * 10.0); }

    FLOAT ReshapeG_R2V(FLOAT g) override { return (FLOAT) (std::min(std::abs(g), 0.5f) * ((g > 0.0f) - (g < 0.0f))); }
    FLOAT ReshapeG_V2R(FLOAT g) override { return (FLOAT) g; }
};

}

#endif /* SIBYL_RESHAPER_ELW_H_ */