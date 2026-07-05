
gcc-12 -Wall -O3  IJK/mmat.c -o mmat_cpu_seq_ijk_gcc
gcc-12 -Wall -O3 -fopenmp -DGCC  IJK/mmat_omp.c -o mmat_cpu_omp_ijk_gcc
gcc-12 -Wall -O3  IKJ/mmat_ikj.c -o mmat_cpu_seq_ikj_gcc
gcc-12 -Wall -O3 -fopenmp -DGCC  IKJ/mmat_ikj_omp.c -o mmat_cpu_omp_ikj_gcc
gcc-12 -Wall -O3 -fopenmp -foffload=nvptx-none -foffload=-misa=sm_75 -fcf-protection=none -fno-stack-protector -no-pie   -DGCC  IJK/mmat_omp_gpu.c -o mmat_gpu_omp_ijk_gcc
gcc-12 -Wall -O3 -fopenacc -foffload=nvptx-none -foffload=-misa=sm_75 -fcf-protection=none -fno-stack-protector -no-pie  -DGPU IJK/mmat_acc.c -o mmat_gpu_acc_ijk_gcc
PGCC=/opt/nvidia/hpc_sdk/Linux_x86_64/24.1/compilers/bin/pgcc
$PGCC -O3 -fast -Minfo=all -mp IJK/mmat.c  -o mmat_cpu_seq_ijk_pgcc
$PGCC -O3 -fopenmp -fast -Minfo=all  IJK/mmat_omp.c -o mmat_cpu_omp_ijk_pgcc
$PGCC -O3 -fast -Minfo=all -mp IKJ/mmat_ikj.c  -o mmat_cpu_seq_ikj_pgcc
$PGCC -O3 -fopenmp -fast -Minfo=all  IKJ/mmat_ikj_omp.c -o mmat_cpu_omp_ikj_pgcc
$PGCC -O3 -fopenmp -fast -Minfo=all  -target=gpu -gpu=cc75 -DPGCC  IJK/mmat_omp_gpu.c -o mmat_gpu_omp_ijk_pgcc
$PGCC -O3 -fast -Minfo=all -acc=multicore -DCPU IJK/mmat_acc.c -o mmat_cpu_acc_ijk_pgcc
$PGCC -O3  -fast -Minfo=all  -acc=gpu -gpu=cc75 -DGPU IJK/mmat_acc.c -o mmat_gpu_acc_ijk_pgcc

source /opt/intel/oneapi/setvars.sh
icx -O3 IJK/mmat.c -o mmat_cpu_seq_ijk_icx
icx -O3 -qopenmp IJK/mmat_omp.c -o mmat_cpu_omp_ijk_icx
icx -O3 IKJ/mmat_ikj.c -o mmat_cpu_seq_ikj_icx
icx -O3 -qopenmp IKJ/mmat_ikj_omp.c -o mmat_cpu_omp_ikj_icx
#icx -O3
#icx -O3

/usr/local/cuda/bin/nvcc -O3 -arch=sm_35 CUDA/matrixmul_sinMca.cu -o matrixmul_sinMca_cuda_bin
/usr/local/cuda/bin/nvcc -O3 -arch=sm_35 CUDA/matrixmul_conMca.cu -o matrixmul_conMca_cuda_bin
/usr/local/cuda/bin/nvcc -O3 -arch=sm_35 CUDA/mmst_cuda.cu -o mmst_cuda_cuda_bin
/opt/nvidia/hpc_sdk/Linux_x86_64/23.1/compilers/bin/nvcc -O3  -arch=sm_35 CUDA/matrixmul_sinMca.cu -o matrixmul_sinMca_cuda_nvcc
/opt/nvidia/hpc_sdk/Linux_x86_64/23.1/compilers/bin/nvcc -O3  -arch=sm_35 CUDA/matrixmul_conMca.cu -o matrixmul_conMca_cuda_nvcc
/opt/nvidia/hpc_sdk/Linux_x86_64/23.1/compilers/bin/nvcc -O3  -arch=sm_35 CUDA/mmst_cuda.cu -o mmst_cuda_cuda_nvcc
