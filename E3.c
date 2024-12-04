#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 12
#define P 9 // Número de productores
#define C 6 // Número de consumidores
#define TAM_BUF (110 * (P+C)) // Tamaño del array T
#define NUM_ITER 18

int buffer[N];
int puntero=0;

int prod_sleep_time_prod, prod_sleep_time_ins, prod_sleep_time_sum;
int cons_sleep_time_cons, cons_sleep_time_ext, cons_sleep_time_sum;

int T[TAM_BUF];

pthread_mutex_t mutex_buffer, mutex_t_par, mutex_t_impar;

int sum_pares = 0;
//En vez de hacer esto se podría asignar 110 elem a cada prod/cons
int ultimo_indice_pares=0;
int ultimo_indice_impares= 1;
int sum_impares = 0;
int consumidos_global=0;


void imprimir_buffer() {
        printf("Contenido del buffer:\n");
        printf("----------------------\n");
        printf("| Índice |   Valor   |\n");
        printf("----------------------\n");
        for (int i = N-1; i >=0; i--) {
            printf("|   %3d  |   %5d   |\n", i, buffer[i]);
        }
        printf("----------------------\n\n");
    }


// Devuelve q si acabaron los productores
int acabna_prod() {
    int finished = (consumidos_global >= P * NUM_ITER);
    return finished;
}

void producir(int item){
    buffer[puntero] = item;
    puntero++;
}

// Función del productor
void *productor(void *arg) {
    int id = *(int *)arg;
    int items_producidos_loc = 0;
    printf("(%d) Hola, soy el productor\n", id);
    while (ultimo_indice_pares< TAM_BUF || items_producidos_loc < NUM_ITER ) {
        if (items_producidos_loc < NUM_ITER ){
        // Producción de un item
        sleep(prod_sleep_time_prod);
        int item = rand() % 100; // Genera un número aleatorio entre 0 y 99
        
        pthread_mutex_lock(&mutex_buffer);
        printf("(%d) Hola, soy el productor entrando del mutex, items_prod= %d\n", id, items_producidos_loc);


        // Inclusión en el buffer
        if (puntero < N){


        sleep(prod_sleep_time_ins);
        producir(item);
        printf("(%d) Productor: item %d, valor_puntero %d\n", id, item, puntero);
        imprimir_buffer();
        items_producidos_loc++;

        
        }

        pthread_mutex_unlock(&mutex_buffer);
        printf("(%d) productor saliendo del mutex\n", id);
        }
        //Acceso fuera de región critica solo de lectura-> dentro volvemos a comprobar
        if (ultimo_indice_pares< TAM_BUF){
        // Contribución al sumatorio de los valores de las posiciones pares de T
        int elementos_suma = rand() % 3 + 2; // Entre 2 y 4 elementos a sumar
        pthread_mutex_lock(&mutex_t_par);
        sleep(prod_sleep_time_sum);
        if (ultimo_indice_pares< TAM_BUF){
            for (int i=0; i < elementos_suma && ultimo_indice_pares < TAM_BUF; i++, ultimo_indice_pares+=2) {
                sum_pares+=T[ultimo_indice_pares];
                //sum_pares += T[(id * P) + i];
            }
        
        printf("(%d) Productor contribuyó a la suma de pares: %d, ultimo indice: %d\n", id, sum_pares, ultimo_indice_pares);
        }
        pthread_mutex_unlock(&mutex_t_par);
        }
        
    }
    
    printf("(%d) Saliendo de productor\n", id);
    pthread_exit(NULL);
}

// En una funcion para mas modularidad y poder consumir muchos de golpe
int consumir(){
    puntero--;
    sleep(cons_sleep_time_ext);
    int item = buffer[puntero];
    buffer[puntero] =-1;  
    return item;  
}



void *consumidor(void *arg) {
    int id = *(int *)arg;
    int items_consumidos = 0;
    printf("(%d)Hola, soy el consumidor\n", id);

    while (1) {
        printf("(%d) Esperando mutex, items consumidos %d\n", id, items_consumidos);
        pthread_mutex_lock(&mutex_buffer);
        if (acabna_prod()){
            while(puntero>0){
            int item = consumir();
            printf("Consumidor %d consumió item %d, valor puntero: %d\n", id, item, puntero);
            }
            pthread_mutex_unlock(&mutex_buffer);
        }
        else{
        // Extracción de un item del buffer
        //puede más de un consumidor esperando y justo productor acaba
        if (puntero > 0){

                //printf("(%d) esperando por si no acabaron %d %d\n", id, acabaron_todos_prod, acabna_prod());
                if(acabna_prod()){
                    break;
                }
                printf("(%d) saliendo de espera\n", id);
            
            if (!acabna_prod()){
            
            
                sleep(cons_sleep_time_cons);
                consumir();
                items_consumidos++;
                consumidos_global++;
                
                imprimir_buffer();
                printf("sali de mutex\n");
            }
        }
            pthread_mutex_unlock(&mutex_buffer);
        
        
        
        }

        //Acceso a región crítica sin mutex
        if (ultimo_indice_impares<TAM_BUF+1){
        // Contribución al sumatorio de los valores de las posiciones impares de T
        int elementos_suma = rand() % 3 + 2; // Entre 2 y 4 elementos a sumar
        
        pthread_mutex_lock(&mutex_t_impar);
        sleep(cons_sleep_time_sum);
        //printf("entrando mutex\n");
        if (ultimo_indice_impares<TAM_BUF+1){
            for (int i = 0; i < elementos_suma && ultimo_indice_impares<TAM_BUF+1; i++, ultimo_indice_impares+=2) {
                //sum_impares += T[(id * C) + i];
                sum_impares+=T[ultimo_indice_impares];
            }
            printf("Consumidor %d contribuyó a la suma de impares: %d, ultimo_indice: %d\n", id, sum_impares, ultimo_indice_impares);
        }
        pthread_mutex_unlock(&mutex_t_impar);
        }
        if (acabna_prod() && ultimo_indice_impares>TAM_BUF){
            printf("Saliendo de consumidor %d\n", id);
            pthread_exit(NULL);
        }

    }
    return  NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("Uso: %s <tiempo_prod_producccion>  <tiempo_prod_insercion> <tiempo_prod_suma> <tiempo_cons_extraccion> <tiempo_cons_consumicion> <tiempo_cons_suma> \n", argv[0]);
        return 1;
    }

    prod_sleep_time_prod = atoi(argv[1]);
    prod_sleep_time_ins = atoi(argv[2]);
    prod_sleep_time_sum = atoi(argv[3]);
    cons_sleep_time_ext = atoi(argv[4]);
    cons_sleep_time_cons= atoi(argv[5]);
    cons_sleep_time_sum = atoi(argv[6]);
    srand(time(NULL));

    pthread_mutex_init(&mutex_buffer, 0);
    pthread_mutex_init(&mutex_t_par, 0);
    pthread_mutex_init(&mutex_t_impar, 0);


    pthread_t threads[P + C];
    int ids[P + C];
    for (int  i=0; i<N; i++){
        buffer[i]=-1;
    }
    // Inicialización del array T
    int suma_calc=0;
    for (int i = 0; i < TAM_BUF; i++) {
        T[i] = i / 2;
        suma_calc+=T[i];
    }
    printf("Suma del buffer total: %d, longitud: %d\n", suma_calc, TAM_BUF);
    
    // Creación de hilos productores
    for (int i = 0; i < P; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, productor, (void *)&ids[i]);
    }

    // Creación de hilos consumidores
    for (int i = 0; i < C; i++) {
        ids[i+P] = i;
        pthread_create(&threads[i+P], NULL, consumidor, (void *)&ids[i+P]);
    }

    // Espera a que todos los hilos terminen
    for (int i = 0; i < P + C; i++) {
        pthread_join(threads[i], NULL);
    }

    // Imprimir los resultados finales de la suma de pares e impares
    printf("Suma final de pares: %d\n", sum_pares);
    printf("Suma final de impares: %d\n", sum_impares);
    printf("Suma total: %d\n", sum_pares+ sum_impares);
    printf("Suma del buffer esperada: %d, longitud: %d\n", suma_calc, TAM_BUF);

    pthread_mutex_destroy(&mutex_buffer);
    pthread_mutex_destroy(&mutex_t_par);
    pthread_mutex_destroy(&mutex_t_impar);


    return 0;
}
