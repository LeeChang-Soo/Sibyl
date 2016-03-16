
#include <string>
#include <iostream>
#include <fstream>
#include <memory>

#include <fractal/fractal.h>

#include <rnn/TradeRnn.h>

#include <sibyl/client/Trader.h>
#include <sibyl/client/NetClient.h>

int main(int argc, char *argv[])
{
    if ( (argc != 4 && argc != 5)                   ||
         (argc == 5 && std::string(argv[4]) != "-v") )
    {
        std::cerr << "USAGE: rnnclnt <workspace list> <ipaddress> <port> [ -v ]" << std::endl;
        exit(1);
    }

    std::string path(argv[0]);
    path.resize(path.find_last_of('/'));


    /* ============================================= */
    /*                  Setup sibyl                  */
    /* ============================================= */
    
    using namespace sibyl;
    
    Trader trader;
    trader.Initialize(TimeBounds(       -3600 /* ref       */,
                                 kTimeTickSec /* init      */,
                                        21000 /* stop      */,
                                        22200 /* end       */));    
    trader.model.SetParams(              60.0 /* timeConst */,
                                          1.0 /* rhoWeight */,
                                        0.001 /* rhoInit   */);    
    trader.SetStateLogPaths(path + "/state", "");//path + "/log");

    NetClient netClient(&trader);
    netClient.SetVerbose((argc == 5) && (std::string(argv[4]) == "-v"));
    
    
    /* =============================================== */
    /*                  Setup fractal                  */
    /* =============================================== */
    
    using namespace fractal;
    
    Engine engine;
    
    std::vector<std::unique_ptr<TradeRnn>> vecRnn;
    std::ifstream pathList(argv[1]);
    if (pathList.is_open() == false)
    {
        std::cerr << "<workspace list> inaccessible at " << argv[1] << std::endl;
        exit(1);
    }
    for (std::string workspace; std::getline(pathList, workspace);)
    {
        if (workspace.empty() == true) continue;
        if (workspace[0] != '/') workspace = path + "/" + workspace;
        vecRnn.push_back(std::unique_ptr<TradeRnn>(new TradeRnn()));
        vecRnn.back()->Configure(engine, TradeRnn::RunType::kNetwork, "", workspace);
    }
    std::size_t nRnn = vecRnn.size();
    
    TradeDataSet tradeData;
    unsigned long inputDim  = tradeData.reshaper.GetInputDim ();
    unsigned long outputDim = tradeData.reshaper.GetTargetDim();
    
    unsigned long nUnroll = 2;
    unsigned long nStream = 0; // will be set below
    
    bool is_init = true;

    
    /* ===================================== */
    /*                  Run                  */
    /* ===================================== */

    /* Connect to server */
    if (0 != netClient.Connect(argv[2], argv[3])) exit(1);

    /* Network main loop */
    while (true)
    {
        /* Receive data and fill Portfolio entries */
        if (0 != netClient.RecvNextTick()) break;
        
        /* Initialize nUnroll & nStream */
        if(is_init == true)
        {
            nStream = trader.portfolio.items.size();
            for (auto &pRnn : vecRnn)
                pRnn->InitUnrollStream(nUnroll, nStream);
            is_init = false;
        }
        
        // Rnn is run only between 09:00:int and 14:50:00
        if ( (trader.portfolio.time >= trader.timeBounds.init) &&
             (trader.portfolio.time <  trader.timeBounds.stop) ) 
        {
            /* Retrieve state vector for current frame */
            const auto &vecState = trader.portfolio.GetStateVec();

            /* Generate the input matrix */
            for (auto &pRnn : vecRnn)
            {
                FLOAT *vecIn = pRnn->GetInputVec();
                for (std::size_t codeIdx = 0; codeIdx < nStream; codeIdx++)
                    tradeData.reshaper.State2Vec(vecIn + codeIdx * inputDim, vecState[codeIdx]);
            }

            /* Run RNN */
            for (auto &pRnn : vecRnn)
                pRnn->RunOneFrame();

            /* Allocate 0-filled rewards vector */
            auto &vecReward = trader.model.GetRewardVec(); 

            /* Get gain values from the output matrix */
            for (auto &pRnn : vecRnn)
            {
                FLOAT *vecOut = pRnn->GetOutputVec();
                for (std::size_t codeIdx = 0; codeIdx < nStream; codeIdx++)
                {
                    Reward temp;
                    tradeData.reshaper.Vec2Reward(temp, vecOut + codeIdx * outputDim, vecState[codeIdx].code);
                    vecReward[codeIdx] += temp;
                }
            }
            for (auto &reward : vecReward) reward *= (FLOAT) 1 / nRnn;
            
            /* Send rewards vector back to model */
            trader.model.SetRewardVec(vecReward); 
        }
        
        /* Calculate based on vecState/vecReward and send requests */
        netClient.SendResponse();
    }

    return 0;
}

