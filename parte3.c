#define THREAD_NUM 1
#define SIZE 10

typedef struct Clock { 
   int p[3];
} Clock;

typedef struct args {
    int processo;
    Clock clock;
    pthread_cond_t condFullEntrada;
    pthread_cond_t condEmptyEntrada;
    pthread_cond_t condFullSaida;
    pthread_cond_t condEmptySaida;
    pthread_mutex_t mutexEntrada;
    pthread_mutex_t mutexSaida;
} Args;

Clock filaEntrada0[SIZE]; //filas do processo 0
Clock filaSaida0[SIZE];

Clock filaEntrada1[SIZE]; //filas do processo 1
Clock filaSaida1[SIZE];
    
Clock filaEntrada2[SIZE]; //filas do processo 2
Clock filaSaida2[SIZE];

void printClock(Clock *clock, int processo) {
   printf("Process: %d, Clock: (%d, %d, %d)\n", processo, clock->p[0], clock->p[1], clock->p[2]);
}

void Event(int pid, Clock *clock){
   clock->p[pid]++;   
}


void Send(int origem, int destino, Clock *clock){
   clock->p[origem]++;  //atualiza o clock
   int * mensagem;
   mensagem = calloc (3, sizeof(int));
   
   for (int i = 0; i < 3; i++) {
         mensagem[i] = clock->p[i];
   }

   MPI_Send(mensagem, 3, MPI_INT, destino, origem, MPI_COMM_WORLD);
   
   free(mensagem);
}


void Receive(int origem, int destino){
   int * mensagem;
   mensagem = calloc (3, sizeof(int));
   Clock *clock = (Clock*)malloc(sizeof(Clock));
   
   MPI_Recv(mensagem, 3,  MPI_INT, origem, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   
   for (int i = 0; i < 3; i++) { 
        clock->p[i] = mensagem[i];
   }
   
   
   
   free(mensagem);
}


void addFilaEntrada() {
    
}




void* threadRelogio(void* arg) {
    int idProcesso = (int) arg;
    while(1) {
        
    }
    return NULL;
}

void* threadSaida(void* arg) {
    
}

void* threadEntrada(void* arg) {
    
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
    
    //argumentos para a threadRelogio
    Args *argsRelogio = (Args*)malloc(sizeof(Args));
    argsRelogio
    
    //argumentos para a threadEntrada
    Args *argsEntrada = (Args*)malloc(sizeof(Args));
    argsEntrada->processo = p;
    argsEntrada->condFull = condFullEntrada;
    argsEntrada->condEmpty = condEmptyEntrada;
    argsEntrada->mutexEntrada = mutexEntrada;
    pthread_cond_init(&(argsEntrada->condFull), NULL);
    pthread_cond_init(&(argsEntrada->Empty), NULL);
    pthread_mutex_init(&(argsEntrada->mutex), NULL);
    
    //argumentos para a threadSaida
    Args *argsSaida = (Args*)malloc(sizeof(Args));
    argsSaida->processo = p;
    argsSaida->condFull = condFullSaida;
    argsSaida->condEmpty = condEmptySaida;
    argsSaida->mutexEntrada = mutexSaida;
    pthread_cond_init(&(argsSaida->condFull), NULL);
    pthread_cond_init(&(argsSaida->Empty), NULL);
    pthread_mutex_init(&(argsSaida->mutex), NULL);
    
    
    //cria threads
    if (pthread_create(&tRelogio, NULL, &threadRelogio, (void*) p) != 0) { //cria thread Relogio
        perror("Failed to create the thread");
    }  
    
    
    if (pthread_create(&tEntrada, NULL, &threadEntrada, (void*) i) != 0) { //cria thread de entrada
        perror("Failed to create the thread");
    }  
    

    if (pthread_create(&tSaida, NULL, &threadSaida, (void*) i) != 0) { //cria thread de saida
        perror("Failed to create the thread");
    }  

    
    
    
    //join das threads 
    if (pthread_join(threadRelogio, NULL) != 0) { //join thread Relogio
        perror("Failed to join the thread");
    }  
    
    for (i = 0; i < THREAD_NUM*2; i++){  
        if (pthread_join(threadsEntrada[i], NULL) != 0) { //join threads entrada
            perror("Failed to join the thread");
        }  
    }
    
    for (i = 0; i < THREAD_NUM*2; i++){  
        if (pthread_join(threadsSaida[i], NULL) != 0) { //join threads saida
            perror("Failed to join the thread");
        }  
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
