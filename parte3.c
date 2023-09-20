#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h> 

// * Compilação: mpicc -o parte3 parte3.c
// * Execução:   mpiexec -n 3 ./parte3

#define SIZE 10

typedef struct Clock { 
   int p[3];
} Clock;

typedef struct mensagem { 
    Clock clock;
    int destino;
    int origem;
} Mensagem;

typedef struct args_entrada { //argumentos da thread de entrada
    int processo;
    int filaEntradaCont;
    pthread_cond_t condFullEntrada;
    pthread_cond_t condEmptyEntrada;
    pthread_mutex_t mutexEntrada;
    Clock filaEntrada[SIZE];
} Args_entrada;

typedef struct args_saida { //argumentos da thread de saída
    int processo;
    int filaSaidaCont;
    pthread_cond_t condFullSaida;
    pthread_cond_t condEmptySaida;
    pthread_mutex_t mutexSaida;
    Mensagem filaSaida[SIZE];
} Args_saida;

typedef struct args_relogio { //argumentos da thread de relógio
    int processo;
    Clock clock;
    Args_entrada argsEntrada;
    Args_saida argsSaida;
}Args_relogio;

int tempoEspera = 1;

void printClock(Clock *clock, int processo) {
   printf("Process: %d, Clock: (%d, %d, %d)\n", processo, clock->p[0], clock->p[1], clock->p[2]);
}

void Event(int pid, Clock *clock){
   clock->p[pid]++;   
}


void Send(int origem, int destino, Clock *clock){
   clock->p[origem]++;  //atualiza o clock
   int * valoresClock;
   valoresClock = calloc (3, sizeof(int));
   
   for (int i = 0; i < 3; i++) {
        valoresClock[i] = clock->p[i];
   }

   MPI_Send(valoresClock, 3, MPI_INT, destino, MPI_ANY_TAG, MPI_COMM_WORLD);
   
   free(valoresClock);
}


Clock* Receive(){
   int *valoresClock;
   valoresClock = calloc (3, sizeof(int));
   Clock *clock = (Clock*)malloc(sizeof(Clock));
   
   MPI_Recv(valoresClock, 3,  MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   
    for (int i = 0; i < 3; i++) {
        clock->p[i] = valoresClock[i];
    }


   free(valoresClock);
   return clock;
}


void insereFilaSaida(void* arg, int origem, int destino) {
    Args_relogio *argsRelogio = arg;
    Args_entrada argsEntrada = argsRelogio->argsEntrada;
    Args_saida argsSaida = argsRelogio->argsSaida;

    pthread_mutex_lock(&argsSaida.mutexSaida); //o lock é feito aqui por causa do clock
        
    while(argsSaida.filaSaidaCont == SIZE) { //enquanto a fila estiver cheia espere
        pthread_cond_wait(&(argsSaida.condFullSaida), &(argsSaida.mutexSaida));
    }
    
    //cria a mensagem
    Mensagem *mensagem = (Mensagem*)malloc(sizeof(Mensagem));
    mensagem->clock = argsRelogio->clock;
    mensagem->origem = origem;
    mensagem->destino = destino;
    
    //insere na fila
    argsSaida.filaSaida[argsSaida.filaSaidaCont] = *mensagem;
        
    pthread_mutex_unlock(&argsSaida.mutexSaida); // faz o unlock da fila
    pthread_cond_signal(&(argsSaida.condEmptySaida)); //fila não está mais vazia
    free(mensagem);
}

void retiraFilaEntrada(void* arg) {
    Args_relogio *argsRelogio = arg;
    Args_entrada argsEntrada = argsRelogio->argsEntrada;
    Args_saida argsSaida = argsRelogio->argsSaida;

    pthread_mutex_lock(&argsEntrada.mutexEntrada); //faz o lock na fila de entrada
    
    while(argsEntrada.filaEntradaCont == 0) { //enquanto estiver vazia espere
        pthread_cond_wait(&(argsEntrada.condEmptyEntrada), &(argsEntrada.mutexEntrada));
    }
    
    //tira do começo da fila
    Clock clock = argsEntrada.filaEntrada[0];
    for (int i = 0; i < (argsEntrada.filaEntradaCont) -1; i++) {
        argsEntrada.filaEntrada[i] = argsEntrada.filaEntrada[i+1];
    }
    
    for (int i = 0; i < 3; i++) { //atualiza o clock da thread relogio
        if(clock.p[i] > argsRelogio->clock.p[i]) {
            argsRelogio->clock.p[i] = clock.p[i];
        }
    }
    
    pthread_mutex_unlock(&argsEntrada.mutexEntrada); //faz o unlock na fila de entrada
    pthread_cond_signal(&(argsEntrada.condFullEntrada)); //fila não está mais cheia
}

void* threadRelogio(void* arg) {
    Args_relogio *args = arg;
    if (args->processo == 0) {
        Event(0, &args->clock);
        printClock(&args->clock, 0);
        
        insereFilaSaida((void*) args, 0, 1);
        printClock(&args->clock, 0);
        
        retiraFilaEntrada((void*) args);
        printClock(&args->clock, 0);
        
        insereFilaSaida((void*) args, 0, 2);
        printClock(&args->clock, 0);
        
        retiraFilaEntrada((void*) args);
        printClock(&args->clock, 0);
        
        insereFilaSaida((void*) args, 0, 1);
        printClock(&args->clock, 0);

        Event(0, &args->clock);
        printClock(&args->clock, 0);
    }
    
    if (args->processo == 1) {
        insereFilaSaida((void*) args, 1, 0);
        printClock(&args->clock, 1);

        retiraFilaEntrada((void*) args);
        printClock(&args->clock, 1);

        retiraFilaEntrada((void*) args);
        printClock(&args->clock, 1);
    }

    if (args->processo == 2) {
        Event(2, &args->clock);
        printClock(&args->clock, 2);

        insereFilaSaida((void*) args, 2, 0); 
        printClock(&args->clock, 2);

        retiraFilaEntrada((void*) args);
        printClock(&args->clock, 2);
    }
    return NULL;
}

void* threadSaida(void* arg) {
    Args_saida *args = arg;
    while(1) {
        pthread_mutex_lock(&(args->mutexSaida));
        
        while(args->filaSaidaCont == 0) {
            pthread_cond_wait(&(args->condEmptySaida), &(args->mutexSaida));
        }
        
        Mensagem *mensagem = &args->filaSaida[0];
        
        Send(mensagem->origem, mensagem->destino, &mensagem->clock);
        
        for(int i = 0; i < (args->filaSaidaCont) -1; i++) {
            args->filaSaida[i] = args->filaSaida[i+1];
        }
        (args->filaSaidaCont)--;
        
        pthread_mutex_unlock(&(args->mutexSaida));
        pthread_cond_signal(&(args->condFullSaida));
    }
    return NULL;
}

void* threadEntrada(void* arg) {
    Args_entrada *args = arg;
    while(1) {
        Clock *clock = Receive();
        pthread_mutex_lock(&(args->mutexEntrada));
        
        while(args->filaEntradaCont == SIZE) {
            pthread_cond_wait(&(args->condFullEntrada), &(args->mutexEntrada));
        }
        
        args->filaEntrada[args->filaEntradaCont] = *clock;
        (args->filaEntradaCont)++;
        
        pthread_mutex_unlock(&(args->mutexEntrada));
        pthread_cond_signal(&(args->condEmptyEntrada));
    }
    return NULL;
}


void processo(int p) {
    Clock clock = {{0,0,0}};
    
    pthread_t tSaida; 
    pthread_t tEntrada;
    pthread_t tRelogio;
    
    pthread_mutex_t mutexEntrada; //mutex da fila de entrada
    pthread_mutex_t mutexSaida; //mutex da fila de saída
    
    pthread_cond_t condFullEntrada; 
    pthread_cond_t condEmptyEntrada;
    
    pthread_cond_t condFullSaida;
    pthread_cond_t condEmptySaida;
    
    int filaEntradaCont = 0; //contador da fila de entrada
    int filaSaidaCont = 0; //contador da fila de sa[ida]
    
    Clock filaEntrada[SIZE]; //filas do processo
    Mensagem filaSaida[SIZE];
    
    //inicializações
    pthread_cond_init(&condFullEntrada, NULL);
    pthread_cond_init(&condEmptyEntrada, NULL);
    pthread_cond_init(&condFullSaida, NULL);
    pthread_cond_init(&condEmptySaida, NULL);
    pthread_mutex_init(&mutexEntrada, NULL);
    pthread_mutex_init(&mutexSaida, NULL);

    // argumentos para a thread de entrada
    Args_entrada argsEntrada;
    argsEntrada.processo = p;
    argsEntrada.filaEntradaCont = 0;
    argsEntrada.condFullEntrada = condFullEntrada;
    argsEntrada.condEmptyEntrada = condEmptyEntrada;
    argsEntrada.mutexEntrada = mutexEntrada;

    // argumentos para a thread de saida
    Args_saida argsSaida;
    argsSaida.processo = p;
    argsSaida.filaSaidaCont = 0;
    argsSaida.condFullSaida = condFullSaida;
    argsSaida.condEmptySaida = condEmptySaida;
    argsSaida.mutexSaida = mutexSaida;

    // argumentos para a thread de relogio
    Args_relogio *argsRelogio = (Args_relogio*)malloc(sizeof(Args_relogio));
    argsRelogio->clock = clock;
    argsRelogio->argsEntrada = argsEntrada;
    argsRelogio->argsSaida = argsSaida;
    

    //cria threads
    if (pthread_create(&tRelogio, NULL, &threadRelogio, (void*) argsRelogio) != 0) { //cria thread Relogio
        perror("Failed to create the thread");
    }     
    if (pthread_create(&tEntrada, NULL, &threadEntrada, (void*) &argsEntrada) != 0) { //cria thread de entrada
        perror("Failed to create the thread");
    }  
    if (pthread_create(&tSaida, NULL, &threadSaida, (void*) &argsSaida) != 0) { //cria thread de saida
        perror("Failed to create the thread");
    }  
    
    //join das threads 
    if (pthread_join(tRelogio, NULL) != 0) { //join thread Relogio
        perror("Failed to join the thread");
    }  
    if (pthread_join(tEntrada, NULL) != 0) { //join threads entrada
        perror("Failed to join the thread");
    }  
    if (pthread_join(tSaida, NULL) != 0) { //join threads saida
        perror("Failed to join the thread");
    }  

    free(argsRelogio);
}



int main(void) {
   int my_rank;               

   MPI_Init(NULL, NULL); 
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 

   if (my_rank == 0) { 
      processo(0);
   } else if (my_rank == 1) {  
      processo(1);
   } else if (my_rank == 2) {  
      processo(2);
   }

   /* Finaliza MPI */
   MPI_Finalize(); 

   return 0;
}  /* main */
