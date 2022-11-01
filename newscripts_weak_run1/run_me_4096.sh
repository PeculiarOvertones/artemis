#!/bin/bash -l

if [ $1 == 0 ]
then
  export CUDA_VISIBLE_DEVICES=$SLURM_LOCALID
else
  export CUDA_VISIBLE_DEVICES=$((3-$SLURM_LOCALID))
  export MPICH_OFI_NIC_POLICY=GPU     #NUMA for CPU
fi

EXE=../Bin/main3d.gnu.TPROF.MTMPI.CUDA.GPUCLOCK.ex
INPUTS=../Examples/Tests/weak_scaling/periodicBox_4096GPUs
FLAGS=

if [ $3 == 1 ]
then
  FLAGS="amrex.the_arena_is_managed=0 amrex.use_gpu_aware_mpi=1"
fi

if [ $SLURM_PROCID == 0 ]
then
  echo "CUDA_VISIBLE_DEVICES = $CUDA_VISIBLE_DEVICES"
  echo "MPICH_OFI_NIC_POLICY = $MPICH_OFI_NIC_POLICY"
  echo "FLAGS = $FLAGS"
fi

${EXE} ${INPUTS} ${FLAGS}
