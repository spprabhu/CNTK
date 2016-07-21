//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#pragma once

#include "SyncAction.h"
#include <unordered_map>
#include <memory>
#include <string>
#include "CUDATimer.h"
#include <utility>
#include <set>

namespace Microsoft { namespace MSR { namespace CNTK {


// forward declarations
class ComputationNodeBase;
class FrameRange;
class SwapInAction;
class SwapOutAction;

class Stats
{
public:
    std::string name;
    float computationTime;
    std::vector<float> swapInTimes;
    std::vector<float> swapOutTimes;
    std::vector<Matrix<float>*> buffers;
    std::vector<std::string> dim;
    void PrintStats()
    {
       fprintf(stdout, "%s: Computation time:%f ", name.c_str(), computationTime);
       fprintf(stdout, "Swap times: \n");
       for(int i = 0; i < swapInTimes.size(); i++)
            fprintf(stdout, "For input %s idx %i: (in %f, out %f)\n", dim[i].c_str(), i, swapInTimes[i], swapOutTimes[i]);
    }
};

class SynchronizationManager
{

private:
    std::unordered_map<int, std::vector<SyncAction*> > m_stepNumber2Actions;
    SynchronizationManager(float performanceCosts);
    SynchronizationManager(){};
    static SynchronizationManager* s_synchronizationManager;

    // needed to identify a step
    std::unordered_map<std::string, int> m_stepName2StepNumber; 
    // steps to buffers; all these buffers need to be handled during one synchronization call for a given timestep
    // needed in order to determine dependencies
    std::unordered_map<Matrix<float>*, std::vector<int> > m_buffer2StepNumbers; 
    std::unordered_map<int, Stats*> m_stepNumber2Stats; 

    //these are for managing full memory swapping during the dryrun
    std::unordered_map<Matrix<float>*, SwapInAction*> m_buffer2SwapIn;
    std::unordered_map<Matrix<float>*, SwapOutAction*> m_buffer2SwapOut;
    std::unordered_map<Matrix<float>*, bool> m_buffer2IsFreed;
    std::unordered_map<int, std::vector<Matrix<float>*> > m_stepNumber2Buffer; 

    std::unordered_map<Matrix<float>*, std::pair<float, float> > m_buffer2SwapTime;
    std::set<Matrix<float> *> m_bufferSet; // contains all buffers which have the potential to be swapped (non-sharable)
    std::unordered_map<int, std::pair<float,float> > m_stepNumber2CumulativeSwapInTime;
    float m_performanceCostLimit;

    bool m_isExecuting;

    CUDATimer m_timer;


    enum SynchronizationState
    {
        Uninitialized = 0,
        RegisteringBuffers = 1,
        GatheringRuntimeStatistics = 2,
        FindingSwapOrder = 4,
        CleaningUp = 8,
        ExecutingActions = 16,
    };

    SynchronizationState m_currentState;
    int m_currentStepNumber;

    void FreeBuffersForDryRun(ComputationNodeBase *node, bool isForward);
    void SwapInFreedBuffers(ComputationNodeBase *node, bool isForward);
    void RegisterBuffers(ComputationNodeBase *node, bool isForward);
    void GatherRuntimeStatistics(ComputationNodeBase *node, const size_t idx, const FrameRange& fr, bool isForward);
    void FindSwapOrder();
    void CleanUp();

    void MeasureSwapTime(ComputationNodeBase *node, int stepNumber);
    std::string GetStepName(ComputationNodeBase *node, bool isForward);
    std::string GetBufferName(ComputationNodeBase *node, bool isForward);


public:
    ~SynchronizationManager(){};
    static SynchronizationManager* GetSynchronizationManager(float performanceCostLimit);
    void BeginSynchronizeState(ComputationNodeBase *node, const size_t idx, const FrameRange& fr, bool isForward);
    void EndSynchronizeState(ComputationNodeBase *node, const size_t idx, const FrameRange& fr, bool isForward);
    bool IsExecuting(){ return m_isExecuting; }
    bool m_useMemorySwapping;
    void ClearActionsAndTheirMemory();
	
};


}}}

