/*
Problema dos Fumantes
*/

/*****************************************************************
Desenvolvido por:                                  *
 * Fernando Kenzo Imami Fugihara           RA205067 *
 * Fabio Luiz Ferreira da Silva Junior     RA247079 *
 ****************************************************************/ 

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define RODADAS 10
#define N_FUMANTES 3
#define N_AGENTES 1

/*
 * Thread Agent
 * Thread Smoker 0 ingredientes[0] = (Tabaco)
 * Thread Smoker 1 ingredientes[1] = (Papel)
 * Thread Smoker 2 ingredientes[2] = (Fosforo)
*/

const char *nomeIngrediente[N_FUMANTES] = {"TABACO ", "PAPEL  ", "FOSFORO"};

typedef enum {
    TABACO,
    PAPEL,
    FOSFORO
} ingrediente;

typedef enum {
    ESPERANDO,
    FUMANDO,
} estado_fumante;

typedef enum {
    DISTRIBUINDO,
    AGUARDANDO_FUMANTES,
} estado_agente;

estado_fumante estado_fum[N_FUMANTES];
estado_agente estado_age[N_AGENTES];

// Variáveis Globais - ao trabalhar com threds um dos maiores problemas são as condições de corrida, onde diferentes threads tentam acessar tais variáveis globais.
int ingredientes_na_mesa[2]; // 0-TABACO, 1-PAPEL, 2-FOSFORO
int fumante_atual = -1; // nenhum deles
int rodada_atual = 0;

// semaphores
sem_t mesa_livre_semaphore; // 0 = mesa ocupada, 1 = mesa livre controla quando o agente pode distribuir os ingredientes novamente
sem_t fumante_semaphore[N_FUMANTES]; // um para cada fumante 
sem_t estados_semaphore; // 0 = estados locked 1 = estados livres controla o estado global -> Mutex Lock

void gerar_cena(int temIngrediente) {
    int i;
    printf("=================== BARZINHO DOS FUMANTES ===================\n");
    printf("Rodada: %2d/%d\n", rodada_atual, RODADAS);

    printf("\n╔════════════════════════════════════════════════════════════════════════╗\n");
    printf  ("║                                 Agente                                 ║\n");
    printf  ("║                ╔══════════════════════════════════════╗                ║\n");
    printf  ("║                ║                  ");
    if(temIngrediente){
        for(int i = 0; i < 2; i++){
            if(ingredientes_na_mesa[i] == 0)
                printf("🍂");
            else if(ingredientes_na_mesa[i] == 1)
                printf("📜");
            else if(ingredientes_na_mesa[i] == 2)
                printf("🔥");
        }
    } else
        printf("    ");
    printf  ("                ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║                                      ║                ║\n");
    printf  ("║                ║");
    if (estado_fum[0] == FUMANDO)
        printf("  🚬  ");
    else
        printf("🍂    ");
    printf("          ");
    if (estado_fum[1] == FUMANDO)
        printf("  🚬  ");
    else
        printf("  📜  ");
    printf("          ");
    if (estado_fum[2] == FUMANDO)
        printf("  🚬  ");
    else
        printf("    🔥");
    printf  ("║                ║\n");
    printf  ("║                ╚══════════════════════════════════════╝                ║\n");
    printf  ("║                Fumante 1      Fumante 2       Fumante 3                ║\n");
    printf  ("╚════════════════════════════════════════════════════════════════════════╝\n");
}

void *f_agente(void *v) {
    int agent_id = 0;

    for (rodada_atual = 1; rodada_atual <= RODADAS; rodada_atual++) {
        sem_wait(&mesa_livre_semaphore); // espera mesa livre
        
        sleep(1); // agente prepara os ingredientes
        
        fumante_atual = random() % N_FUMANTES; // em vez de sortear dois ingredientes e depois descobrir qual fumante serve, o agente ja escolhe o fumante
        
        sem_wait(&estados_semaphore); // LOCK trava por dentro para alterar variáveis globais
        estado_age[agent_id] = DISTRIBUINDO;
         
        int fumante_i; 
        int ingrediente_i = 0;
        // Agente distribuindo os ingredientes na mesa exceto o do escolhido para fumar
        for (fumante_i = 0; fumante_i < N_FUMANTES; fumante_i++) {
            if (fumante_i != fumante_atual) {
                ingredientes_na_mesa[ingrediente_i] = fumante_i;
                ingrediente_i += 1;
            }
        }

        gerar_cena(1); // imprime cena atualizada

        estado_age[agent_id] = AGUARDANDO_FUMANTES; // troca estado do agente

        sem_post(&estados_semaphore); // UNLOCK

        // Acorda o fumante que consegue montar o cigarro completo
        sem_post(&fumante_semaphore[fumante_atual]);
    }
    return NULL;
}

void *f_fumante(void *v) {
    int *ptr = (int *) v; // parse para int *
    int id = *ptr; // deref ptr
    for (;;) {
        sem_wait(&fumante_semaphore[id]); // espera sua vez de montar cigarro
        sem_wait(&estados_semaphore); // LOCK

        estado_fum[id] = FUMANDO;

        // FUMANTE pegou os ingredientes e montou o cigarro
        gerar_cena(0);
        sem_post(&estados_semaphore); // UNLOCK

        sleep(2); // fumando o cigarro

        sem_wait(&estados_semaphore); // LOCK
        estado_fum[id] = ESPERANDO;

        // FUMANTE acabou o cigarro
        gerar_cena(0);
        sem_post(&estados_semaphore); //UNLOCK

        // Libera mesa
        sem_post(&mesa_livre_semaphore);
    }
    return NULL;
}

int main() {
    pthread_t thread_agente;
    pthread_t thread_fumantes[N_FUMANTES]; // array com id das threads
    int ids_fumantes[N_FUMANTES];
    int id_agente = 0;
    // Semaphores globais
    sem_init(&mesa_livre_semaphore, 0, 1); // 0 = semaphore é compartilhado entre threads do mesmo processo, 1 = mesa livre
    sem_init(&estados_semaphore, 0, 1); // 0 = semaphore é compartilhado entre threads do mesmo processo, 1 = mesa livre

    estado_age[0] = AGUARDANDO_FUMANTES;

    // Todos fumantes esperando inicialmente
    int i;
    for (i = 0; i < N_FUMANTES; i++) {
        sem_init(&fumante_semaphore[i], 0, 0); // todos fumantes esperando
        estado_fum[i] = ESPERANDO;
    }

    // Cria 3 threads para cada fumante
    for (i = 0; i < N_FUMANTES; i++) {
        ids_fumantes[i] = i; // guarda id
        pthread_create(&thread_fumantes[i], NULL, f_fumante, (void *) &ids_fumantes[i]); // (void *) &ids_fumantes[i] será lido em f_fumante
    }

    // Cria 1 thread para agente
    pthread_create(&thread_agente, NULL, f_agente, NULL);

    pthread_join(thread_agente, NULL);

    return 0;
}