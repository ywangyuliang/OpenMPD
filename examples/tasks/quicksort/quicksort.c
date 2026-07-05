#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

#define MAX 70000000
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
		/* do nothing */
	}
	while(rightPointer >0 && Array[--rightPointer] > pivot) {
		/* do nothing */
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
	quickSort(left, partitionPoint-1);
	quickSort(partitionPoint+1, right);
  }
}

int main() {
  int i;
  double start_time, run_time;
  struct timeval t1, t2;

  srand(14);
  for (i=0; i<MAX; i++)
    Array[i] = (int) (rand() %MAX);

  printf("Input Array;  ");
  if (MAX < 1000)
	display();
  else
	display_pocos();

  printf("----> QuickSort Secuencial. Num_elem = %d\n", MAX);

#ifdef _OPENMP
  start_time = omp_get_wtime();
#else
  gettimeofday(&t1, NULL);
#endif
  quickSort(0, MAX-1);
#ifdef _OPENMP
  run_time = omp_get_wtime() - start_time;
#else
  gettimeofday(&t2, NULL);
  run_time = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));
#endif

  printf("\n ******* Tiempo de QuickSort Secuencial = %f\n", run_time);

  printf("\n");
  printf("Output Array;  ");
  if (MAX < 1000)
	display();
  else
	display_pocos();

 return (EXIT_SUCCESS);
}
