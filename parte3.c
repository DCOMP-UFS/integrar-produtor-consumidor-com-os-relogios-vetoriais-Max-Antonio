#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h> 

// * Compilação: mpicc -o parte3 parte3.c
// * Execução:   mpiexec -n 3 ./parte3


#define THREAD_NUM 1
#define SIZE 10

typedef struct Clock { 
   int p[3];
} Clock;

typedef struct mensagem {
    Clock clock;
    int destino;
    int origem;
} Mensagem;

typedef struct args {
    int processo;
    int filaEntradaCont;
    int filaSaidaCont;
    Clock clock;
    pthread_cond_t condFullEntrada;
    pthread_cond_t condEmptyEntrada;
    pthread_cond_t condFullSaida;
    pthread_cond_t condEmptySaida;
    pthread_mutex_t mutexEntrada;
    pthread_mutex_t mutexSaida;
    Clock *filaEntrada;
    Mensagem *filaSaida;
} Args;


int tempoEspera = 1;

void printClock(Clock *clock, int processo) {
   printf("Process: %d, Clock: (%d, %d, %d)\n", processo, clock->p[0], clock->p[1], clock->p[2]);
}

void Event(int pid, Clock *clock){
   clock->p[pid]++;   
}


void Send(int origem, int destino, Clock *clock){
   clock->p[origem]++;  //atualiza o clock
   int * mensagemClock;
   mensagemClock = calloc (3, sizeof(int));
   
   for (int i = 0; i < 3; i++) {
         mensagemClock[i] = clock->p[i];
   }

   MPI_Send(mensagemClock, 3, MPI_INT, destino, MPI_ANY_TAG, MPI_COMM_WORLD);
   
   free(mensagemClock);
}


Clock* Receive(){
   int * mensagemClock;
   mensagemClock = calloc (3, sizeof(int));
   Clock *clock = (Clock*)malloc(sizeof(Clock));
   
   MPI_Recv(mensagemClock, 3,  MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   
   for (int i = 0; i < 3; i++) { 
        clock->p[i] = mensagemClock[i];
   }
   
   free(mensagemClock);
   return clock;
}


void insereFilaSaida(void* arg, int origem, int destino) {
    Args *args = arg;
    pthread_mutex_lock(&args->mutexSaida); //o lock é feito aqui por causa do clock
        
    while(args->filaSaidaCont == SIZE) {
        pthread_cond_wait(&(args->condFullEntrada), &(args->mutexEntrada));
    }
        
    Mensagem *mensagem = (Mensagem*)malloc(sizeof(Mensagem));
    mensagem->clock = args->clock;
    mensagem->origem = origem;
    mensagem->destino = destino;
        
    args->filaSaida[args->filaSaidaCont] = *mensagem;
        
    pthread_mutex_unlock(&args->mutexSaida);
    pthread_cond_signal(&(args->condEmptySaida));
    free(mensagem);
}

void retiraFilaEntrada(void* arg) {
    Args *args = arg;
    pthread_mutex_lock(&args->mutexEntrada);
    
    while(args->filaEntradaCont == 0) {
        pthread_cond_wait(&(args->condEmptySaida), &(args->mutexEntrada));
    }
    
    Clock clock = args->filaEntrada[0];
    for (int i = 0; i < (args->filaEntradaCont) -1; i++) {
        args->filaEntrada[i] = args->filaEntrada[i+1]
    }
    
    for (int i = 0; i < 3; i++) {
        if(clock->p[i] > args->clock->p[i]) {
            args->clock->p[i] = clock->p[i];
        }
    }
    
    pthread_mutex_unlock(&args->mutexEntrada);
    pthread_cond_signal(&(args->condFullEntrada));
}

void* threadRelogio(void* arg) {
    Args *args = arg;
    if (args->processo = 0) {
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
        
        insereFilaSaida((void*) args, 0, )
        
    }
    
    if (args->processo = 1) {
        
    }
    if (args->processo = 2) {
        
    }
    
    return NULL;
}

void* threadSaida(void* arg) {
    Args *args = arg;
    while(1) {
        pthread_mutex_lock(&(args->mutexSaida));
        
        while(args->filaSaidaCont == 0) {
            pthread_cond_wait(&(args->condEmptySaida), &(args->mutexSaida));
        }
        
        Mensagem *mensagem = (Mensagem*)malloc(sizeof(Mensagem));
        mensagem = &args->filaSaida[0];
        
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
    Args *args = arg;
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
    
    pthread_mutex_t mutexEntrada; //mutex do processo 0
    pthread_mutex_t mutexSaida;
    
    pthread_cond_t condFullEntrada;
    pthread_cond_t condEmptyEntrada;
    
    pthread_cond_t condFullSaida;
    pthread_cond_t condEmptySaida;
    
    int filaEntradaCont = 0;
    int filaSaidaCont = 0;
    
    Clock filaEntrada[SIZE]; //filas do processo
    Mensagem filaSaida[SIZE];
    
    //argumentos para a threadRelogio
    Args *argsRelogio = (Args*)malloc(sizeof(Args));
    argsRelogio->clock;
    argsRelogio->processo = p;
    argsRelogio-> filaEntradaCont = filaEntradaCont;
    argsRelogio-> filaSaidaCont = filaSaidaCont;
    argsRelogio->condEmptyEntrada = condEmptyEntrada;
    argsRelogio->condFullEntrada = condFullEntrada;
    argsRelogio->condEmptySaida = condEmptySaida;
    argsRelogio->condFullSaida = condFullSaida;
    argsRelogio->mutexEntrada = mutexEntrada;
    argsRelogio->mutexSaida = mutexSaida;
    argsRelogio->filaEntrada = filaEntrada;
    argsRelogio->filaSaida = filaSaida;
    
    pthread_cond_init(&(argsRelogio->condFullEntrada), NULL);
    pthread_cond_init(&(argsRelogio->condEmptyEntrada), NULL);
    pthread_cond_init(&(argsRelogio->condFullSaida), NULL);
    pthread_cond_init(&(argsRelogio->condEmptySaida), NULL);
    pthread_mutex_init(&(argsRelogio->mutexEntrada), NULL);
    pthread_mutex_init(&(argsRelogio->mutexSaida), NULL);
    

    //cria threads
    if (pthread_create(&tRelogio, NULL, &threadRelogio, (void*) argsRelogio) != 0) { //cria thread Relogio
        perror("Failed to create the thread");
    }  
    
    
    if (pthread_create(&tEntrada, NULL, &threadEntrada, (void*) argsRelogio) != 0) { //cria thread de entrada
        perror("Failed to create the thread");
    }  
    

    if (pthread_create(&tSaida, NULL, &threadSaida, (void*) argsRelogio) != 0) { //cria thread de saida
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
