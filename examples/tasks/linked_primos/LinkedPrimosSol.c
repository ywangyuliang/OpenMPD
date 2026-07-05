#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <omp.h>
#include <sys/time.h>

struct linked_list {
	int value;
	int n_prim;
	struct linked_list *next;
};
typedef struct linked_list my_data;

/* Count the prime numbers up to n */
int primos (int n)
{
 int i,j,es_primo;
 int total=1;

 for (i=3; i<=n; i+=2) {
	es_primo = 1;
	for (j=3; j<i; j+=2) {
		if ((i%j) == 0) {
			es_primo = 0;
			break;
		}
	}
        total = total+es_primo;
 }
return(total);
}

int main(int argc, char **argv)
{
	struct timeval t1, t2;

	double start_time, run_time;
	my_data *previous, *ptr, *current, *head_of_list;
	int n_elementos, i;
	int val_ant;
	if (argc <3){
                printf("Uso: Ejecutable no_elementos elementos_a_imprimir\n");
                return(0);
        }
	n_elementos=atoi(argv[1]);
	int imprimir = atoi(argv[2]);

	int num_hilos;
printf("Numero de elementos = %d\n", n_elementos);

/* Create the list and initialize its elements */

	for (i=0; i<n_elementos; i++) {
		if ((ptr = (my_data *)malloc(sizeof(my_data))) == NULL) {
			perror ("ptr");
			exit(-1);
		}
		if (i==0)
			head_of_list=ptr;
		else
			previous->next = ptr;
		ptr->value = rand() %20000;
		ptr->n_prim = 0;
		ptr->next = NULL;
		previous = ptr;
	}
/* Process list elements */

#ifdef _OPENMP
	start_time = omp_get_wtime();
#else
	gettimeofday(&t1, NULL);
#endif
	current = head_of_list;

#pragma omp parallel
{
   #pragma omp single
   {
	num_hilos = omp_get_num_threads();
	while(current!=NULL) {
		#pragma omp task firstprivate(current)
		{
			int return_value;
			return_value = primos(current->value);
			current->n_prim = return_value;
		}
		current = current->next;
	} /* end while */
    } /* end single */

} /* end parallel */
#ifdef _OPENMP
	run_time = omp_get_wtime() - start_time;
#else
	gettimeofday(&t2, NULL);
	run_time = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));
#endif
printf("\n Tiempo version paralela = %f s, con #hilos=%d\n", run_time,num_hilos);

/* Print a few final list elements */
printf("********** Imprimir el resultado\n");
	current = head_of_list;
	imprimir=10;
	while (current!=NULL && imprimir>=0){
			printf("valor = %d ", current->value);
			printf("n_prim = %d\n", current->n_prim);
			imprimir--;
			current = current->next;
	}
	printf("\n");
}
