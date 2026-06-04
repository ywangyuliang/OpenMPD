#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <omp.h>

#ifndef _OPENMP
#define omp_get_num_threads() 1
#endif

#define MAX 70000000
//#define MAX 500

int Array[MAX];

void display() {
   int i;
   printf("[");
   for (i=0; i<MAX; i++) 
      printf("%d ", Array[i]);
   printf("]\n");
}

void display_pocos() {
   printf("[");
   printf("%d, %d, %d, %d, %d ---- %d, %d, %d, %d, %d", Array[0], Array[1], Array[2], Array[3], Array[4],
                             Array[MAX-5],Array[MAX-4], Array[MAX-3], Array[MAX-2], Array[MAX-1]);
   printf("]\n");
}

void swap (int n1, int n2) {
   int tmp = Array[n1];
   Array[n1] = Array[n2];
   Array[n2] = tmp;
}

int partition (int left, int right, int pivot) {
  int leftPointer = left-1;
  int rightPointer = right;

  while (true) {
	while(Array[++leftPointer] < pivot) {
 		// do nothing
    	} 
    	while(rightPointer >0 && Array[--rightPointer] > pivot) {
		// do nothing
    	}
   	if (leftPointer >= rightPointer) {
    		break;
   	}
	else {
		swap(leftPointer, rightPointer);
     	}
  }
  swap (leftPointer, right);
  return leftPointer;
}

void quickSort (int left, int right) {

  if (right-left <=0)
	return;
  
  else {
	int pivot = Array[right];
	int partitionPoint = partition(left, right, pivot);
#ifdef _OPENMP
	#pragma omp task firstprivate(left,partitionPoint) 
#endif
	quickSort(left, partitionPoint-1);
#ifdef _OPENMP
	#pragma omp task firstprivate(right,partitionPoint)
#endif
	quickSort(partitionPoint+1, right);
  }
}

int main() {
  int i;
  double start_time, run_time;

  srand(14);
  for (i=0; i<MAX; i++)
    Array[i] = (int) (rand() %MAX);

  printf("Input Array;  ");
  if (MAX < 1000)
	display();
  else
	display_pocos();

  printf("----> QuickSort Paralelo. Num_elem = %d\n", MAX);

#ifdef _OPENMP
  start_time = omp_get_wtime();
#endif

#ifdef _OPENMP
#pragma omp parallel
#endif
{
#ifdef _OPENMP
  #pragma omp single
#endif
  { printf("Num_hilos = %d\n", omp_get_num_threads());
    quickSort(0, MAX-1);
  }
}
#ifdef _OPENMP
  run_time = omp_get_wtime() - start_time;
  printf("\n ******* Tiempo de QuickSort Paralelo = %f\n", run_time);
#endif

  printf("\n");
  printf("Output Array;  ");
  if (MAX < 1000)
	display();
  else
	display_pocos();

 return (EXIT_SUCCESS);
} 
