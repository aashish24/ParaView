/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQRandomCells.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkType.h"
#include "vtkCellType.h"

#include "CellCopier.h"
#include "PolyDataCellCopier.h"
#include "UnstructuredGridCellCopier.h"
#include "SQMacros.h"
#include "IdBlock.h"
#include "Numerics.hxx"
#include "Tuple.hxx"

#include <algorithm>
#include <set>

typedef std::pair<std::set<unsigned long long>::iterator,bool> SetInsert;

#include <cstdlib>
#include <ctime>

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

// #define SQTK_DEBUG


//*****************************************************************************
void dumpBlocks(IdBlock *bins, int n)
{
  for (int i=0; i<n; ++i)
    {
    std::cerr << "proc " << i << " has " << bins[i] << std::endl;
    }
}

//*****************************************************************************
int findProcByCellId(unsigned long long cellId, IdBlock *bins, int s, int e)
{
  // binary search for rank who owns the given cell id. This
  // won't handle non-contiguous cell id ranges but will handle
  // processes with no cells.

  // if (s>e || e<s)
  //   {
  //   sqErrorMacro(std::cerr,"Index error s=" << s << " e=" << e << ".");
  //   return -1;
  //   }

  int m=(s+e)/2;

  // skip procs with no cells
  while (bins[m].empty())
    {
    if (cellId<bins[m].first())
      {
      if (m<=s){ break; }
      --m; // move left
      }
    else
      {
      if (m>=e){ break; }
      ++m; // move right
      }
    }

  if (bins[m].contains(cellId))
    {
    //std::cerr << "proc=" << m << " owns " << cellId << "." << std::endl;
    return m;
    }
  else
  if ((cellId<bins[m].first())&&(s!=e))
    {
    return findProcByCellId(cellId,bins,s,m-1);
    }
  else
  if ((cellId>(bins[m].last()-1))&&(s!=e))
    {
    return findProcByCellId(cellId,bins,m+1,e);
    }

  // not found, should always be found
  sqErrorMacro(std::cerr,"Error: CellId=" << cellId << " was not found.");
  return -1;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQRandomCells);

//----------------------------------------------------------------------------
vtkSQRandomCells::vtkSQRandomCells()
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQRandomCells::vtkSQRandomCells" << std::endl;
  #endif

  this->SampleSize=0;
  this->Seed=-1;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQRandomCells::~vtkSQRandomCells()
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQRandomCells::~vtkSQRandomCells" << std::endl;
  #endif
}

//----------------------------------------------------------------------------
int vtkSQRandomCells::RequestInformation(
    vtkInformation * /*req*/,
    vtkInformationVector ** /*inInfos*/,
    vtkInformationVector *)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQRandomCells::RequestInformation" << std::endl;
  #endif

  // TODO extract bounds and set if the input data set is present.
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQRandomCells::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfos,
    vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQRandomCells::RequestData" << std::endl;
  #endif


  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  vtkDataSet *source
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // validate input, output, and parameters
  if (source==NULL)
    {
    vtkErrorMacro("Empty input.");
    return 1;
    }

  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  vtkDataSet *output
    = dynamic_cast<vtkDataSet*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (output==NULL)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }

  if (this->SampleSize<1)
    {
    vtkErrorMacro("Number of cells must be greater than 0.");
    output->Initialize();
    return 1;
    }

  // create the cell coppier.
  CellCopier *copier;
  if (dynamic_cast<vtkPolyData*>(source))
    {
    copier=new PolyDataCellCopier;
    }
  else
  if (dynamic_cast<vtkUnstructuredGrid*>(source))
    {
    copier=new UnstructuredGridCellCopier;
    }
  else
    {
    vtkErrorMacro("Unsupported dataset type " << source->GetClassName() << ".");
    return 1;
    }
  copier->Initialize(source,output);

  int worldSize=1;
  int worldRank=0;
  int mpiOk=0;
  #ifndef SQTK_WITHOUT_MPI
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    MPI_Comm_size(MPI_COMM_WORLD,&worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldRank);
    }
  #endif

  // count of the cells we own.
  unsigned long long nLocalCells=source->GetNumberOfCells();

  // number and local cell ids of cells which we own which
  // are to be passed through to the output.
  unsigned long long nCellsToPass=0;
  std::vector<unsigned long long> cellsToPass;

  const int masterRank=(worldSize==1?0:1);

  // divi up the work, all process must participate.
  if (worldRank==masterRank)
    {
    // get counts of all cells.
    unsigned long long *nRemoteCells
      = (unsigned long long *)malloc(worldSize*sizeof(unsigned long long));
    if (mpiOk)
      {
      #ifndef SQTK_WITHOUT_MPI
      MPI_Gather(
          &nLocalCells,
          1,
          MPI_UNSIGNED_LONG_LONG,
          &nRemoteCells[0],
          1,
          MPI_UNSIGNED_LONG_LONG,
          masterRank,
          MPI_COMM_WORLD);
      #endif
      }
    else
      {
      nRemoteCells[0]=nLocalCells;
      }

    // construct the cell id block owned by each process.
    unsigned long long nCellsTotal=0;
    IdBlock *remoteCellIds=new IdBlock[worldSize];
    for (int i=0; i<worldSize; ++i)
      {
      remoteCellIds[i].first()=nCellsTotal;
      remoteCellIds[i].size()=nRemoteCells[i];
      nCellsTotal+=nRemoteCells[i];
      }
    free(nRemoteCells);

    // select cells to pass through. assigned to the process who
    // owns them.
    unsigned long long *nAssigned
      = (unsigned long long *)malloc(worldSize*sizeof(unsigned long long));
    for (int i=0; i<worldSize; ++i)
      {
      nAssigned[i]=0ll;
      }
    std::vector<unsigned long long> *assignments=new std::vector<unsigned long long>[worldSize];

    // seed the number generator.
    int seed=(this->Seed<0?((int)time(0)):this->Seed);
    srand(seed);

    std::set<unsigned long long> usedCellIds;
    SetInsert ok;

    // fraction of the available set that the sample
    // covers. values greater than 1 are illegal, sample size
    // will be capped at the number of available cells. We also
    // need to avoid coverage approaching 1 because it will take
    // a long time for the random number generator to hit all values.
    double coverage=((double)this->SampleSize)/((double)nCellsTotal);
    // if (coverage>0.9)
    //   {
    //   vtkWarningMacro(
    //     << "Using all cells. Sample size change from "
    //     << this->SampleSize
    //     << " to "
    //     << nCellsTotal
    //     << ".");
    //   // assign all to prevent endless spinning while random
    //   // generator tries to cover all values.
    //   for (unsigned long long i=0; i<nCellsTotal; ++i)
    //     {
    //     unsigned long long cellId=i;
    //     // look up its process id
    //     int rank=findProcByCellId(cellId,remoteCellIds,0,worldSize-1);
    //     if (rank<0)
    //       {
    //       vtkErrorMacro("Cell id  " << cellId << " was not found on any process.");
    //       dumpBlocks(remoteCellIds,worldSize);
    //       abort();
    //       }
    //     cellId-=remoteCellIds[rank].first();
    //     assignments[rank].push_back(cellId);
    //     ++nAssigned[rank];
    //     }
    //   }
    // else
    if (coverage>0.75)
      {
      // sample size is large enough that random selection of n unique cells
      // is impractical.
      unsigned long long reducedSampleSize
        = (unsigned long long)std::max(1.0,0.75*(double)nCellsTotal);

      vtkWarningMacro(
        << "Reducing sample size from "
        << this->SampleSize
        << " to "
        << reducedSampleSize
        << ".");

      for (unsigned long long i=0; i<reducedSampleSize; ++i)
        {
        // find an unsed cell id.
        unsigned long long cellId=0;
        do
          {
          cellId
          =(unsigned long long)((double)(nCellsTotal-1)*(double)rand()/(double)RAND_MAX);
          ok=usedCellIds.insert(cellId);
          }
        while (!ok.second);
        // look up its process id
        int rank=findProcByCellId(cellId,remoteCellIds,0,worldSize-1);
        if (rank<0)
          {
          vtkErrorMacro("Cell id  " << cellId << " was not found on any process.");
          dumpBlocks(remoteCellIds,worldSize);
          abort();
          }
        cellId-=remoteCellIds[rank].first();
        assignments[rank].push_back(cellId);
        ++nAssigned[rank];
        }
      }
    else
      {
      // sample size is small enough that selecting n unique cells is
      // paractical.
      for (int i=0; i<this->SampleSize; ++i)
        {
        // find an unsed cell id.
        unsigned long long cellId=0;
        do
          {
          cellId=(unsigned long long)((double)(nCellsTotal-1)*(double)rand()/(double)RAND_MAX);
          ok=usedCellIds.insert(cellId);
          }
        while (!ok.second);
        // look up its process id
        int rank=findProcByCellId(cellId,remoteCellIds,0,worldSize-1);
        if (rank<0)
          {
          vtkErrorMacro("Cell id  " << cellId << " was not found on any process.");
          dumpBlocks(remoteCellIds,worldSize);
          abort();
          }
        cellId-=remoteCellIds[rank].first();
        assignments[rank].push_back(cellId);
        ++nAssigned[rank];
        }
      }
    delete [] remoteCellIds;

    // distribute the assignments
    for (int i=0; i<worldSize; ++i)
      {
      if (i==masterRank)
        {
        nCellsToPass=nAssigned[i];
        cellsToPass=assignments[i];
        continue;
        }
      if (mpiOk)
        {
        #ifndef SQTK_WITHOUT_MPI
        MPI_Send(
            &nAssigned[i],
            1,
            MPI_UNSIGNED_LONG_LONG,
            i,
            0,
            MPI_COMM_WORLD);
        MPI_Send(
            &((assignments[i])[0]),
            (int)nAssigned[i], // TODO -- this is a problem with MPI api.
            MPI_UNSIGNED_LONG_LONG,
            i,
            1,
            MPI_COMM_WORLD);
        #endif
        }
      }
    delete [] assignments;
    free(nAssigned);
    }
  else
    {
    // send a count of the cells we own.
    if (mpiOk)
      {
      #ifndef SQTK_WITHOUT_MPI
      MPI_Gather(
          &nLocalCells,
          1,
          MPI_UNSIGNED_LONG_LONG,
          0,
          0,
          MPI_UNSIGNED_LONG_LONG,
          masterRank,
          MPI_COMM_WORLD);

      // obtain our assignments
      MPI_Status stat;
      MPI_Recv(
          &nCellsToPass,
          1,
          MPI_UNSIGNED_LONG_LONG,
          masterRank,
          0,
          MPI_COMM_WORLD,
          &stat);

      cellsToPass.resize(nCellsToPass);
      MPI_Recv(
          &cellsToPass[0],
          (int)nCellsToPass, // TODO -- this is a problem with MPI api.
          MPI_UNSIGNED_LONG_LONG,
          masterRank,
          1,
          MPI_COMM_WORLD,
          &stat);
      #endif
      }
    }

  // copy cells, assoictaed points and data attributes to the output.
  for (unsigned long long i=0; i<nCellsToPass; ++i)
    {
    copier->Copy(cellsToPass[i]);
    }

  delete copier;

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQRandomCells::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQRandomCells::PrintSelf" << std::endl;
  #endif

  this->Superclass::PrintSelf(os,indent);
}
