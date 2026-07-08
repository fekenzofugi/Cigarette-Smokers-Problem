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
#define N_COMBINADORES 3

/*
 * Thread Agente
 * Thread Fumante 0 ingredientes[0] = (Tabaco)
 * Thread Fumante 1 ingredientes[1] = (Papel)
 * Thread Fumante 2 ingredientes[2] = (Fosforo)
 * Thread Combinador 0 (Tabaco, Papel) Acorda Fumante 2
 * Thread Combinador 1 (Tabaco, Fosforo) Acorda Fumante 1
 * Thread Combinador 2 (Papel Fosforo) Acorda Fumante 0
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

// Variáveis Globais - ao trabalhar com threds um dos maiores problemas são as condições de corrida, onde diferentes threads tentam acessar tais variáveis globais. Protegidas por estados_semaphore
int mesa[2]; // 0-TABACO, 1-PAPEL, 2-FOSFORO
int fumante_atual = -1; // nenhum deles
int rodada_atual = 0;

int ingredientes_presentes[N_FUMANTES]; // ingredientes_presentes[TABACO_I] == 1 significa que o ingrediente Tabaco está sobre mesa nom momento 

// semaphores
sem_t mesa_livre_semaphore; // 0 = mesa ocupada, 1 = mesa livre controla quando o agente pode distribuir os ingredientes novamente
sem_t fumante_semaphore[N_FUMANTES]; // um para cada fumante 
sem_t estados_semaphore; // 0 = estados locked 1 = estados livres controla o estado global -> Mutex Lock
sem_t alerta_combinador_semaphore[N_COMBINADORES]; // um para cada combinador. alerta_combinador_semaphore[0] == 1 agente ativa esse semaphore quando Tabaco esta na mesa, agente toca toda rodada para todos os combinadores e cada combinador ao acordar ira verificar em paralelo se o seu par de ingredientes esta ativo

int combinacoes[N_COMBINADORES][3] = {
    {TABACO, PAPEL,   FOSFORO},
    {TABACO, FOSFORO, PAPEL},
    {PAPEL,  FOSFORO, TABACO},
};

void gerar_cena() {
    int i;
    int id_agente = 0;
    printf("=================== BARZINHO DOS FUMANTES ===================\n");
    printf("Rodada: %2d/%d\n", rodada_atual, RODADAS);

    printf("Agente: %s\n", estado_age[0] == DISTRIBUINDO ? "distribuindo" : "aguardando");

    printf("Mesa:   ");
    if (estado_age[id_agente] == DISTRIBUINDO || fumante_atual == -1) {
        printf("[ %s ] [ %s ]\n", nomeIngrediente[mesa[0]], nomeIngrediente[mesa[1]]);
    } else {
        printf("[ vazia ] [ vazia ]\n");
    }

    printf("\nFumantes:\n");
    for (i = 0; i < N_FUMANTES; i++) {
        printf("  Fumante %d (tem %s): %s\n", i, nomeIngrediente[i],
        estado_fum[i] == FUMANDO ? "🚬" : "esperando");
    }
    return;
}

void *f_agente(void *v) {
    int agent_id = 0;
    int ingrediente1, ingrediente2;

    for (rodada_atual = 1; rodada_atual <= RODADAS; rodada_atual++) {
        sem_wait(&mesa_livre_semaphore); // espera mesa livre
        
        sleep(1); // agente prepara os ingredientes
        
        ingrediente1 = random() % N_FUMANTES; // em vez de sortear dois ingredientes e depois descobrir qual fumante serve, o agente ja escolhe o fumante
        while (ingrediente1 == ingrediente2) {
            ingrediente2 = random() % N_FUMANTES;
        }

        sem_wait(&estados_semaphore); // LOCK trava por dentro para alterar variáveis globais
        estado_age[agent_id] = DISTRIBUINDO;

        // adiciona ingredientes na mesa
        mesa[0] = ingrediente1;
        mesa[1] = ingrediente2;

        fumante_atual = (TABACO + PAPEL + FOSFORO) - ingrediente1 - ingrediente2;

        gerar_cena();

        estado_age[agent_id] = AGUARDANDO_FUMANTES; // troca estado do agente

        sem_post(&estados_semaphore); // UNLOCK

        // Acorda o fumante que consegue montar o cigarro completo
        sem_post(&fumante_semaphore[fumante_atual]);

        // Alerta os 3 combinadores -> Cada combinador ira checar se o seu par esta completo
        int i;
        for (i = 0; i < N_COMBINADORES; i++) {
            sem_post(&alerta_combinador_semaphore[i]);
        }
    }
    return NULL;
}

void *f_combinador(void *v) {
    int *combinacao = (int *) v;
    int ingrediente1 = combinacao[0];
    int ingrediente2 = combinacao[1];
    int ultimo_ingrediente = combinacao[2];

    for (;;) {
        sem_wait(&alerta_combinador_semaphore[ultimo_ingrediente]);
        sem_wait(&estados_semaphore); // lock

        if (ingredientes_presentes[ingrediente1] && ingredientes_presentes[ingrediente2]) { // checa se ingredientes estao presentes na mesa
            ingredientes_presentes[ingrediente1] = 0;
            ingredientes_presentes[ingrediente2] = 0;
            sem_post(&estados_semaphore);
            sem_post(&fumante_semaphore[ultimo_ingrediente]);
        } else {
            sem_post(&estados_semaphore); // unlock
        }
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
        gerar_cena();
        sem_post(&estados_semaphore); // UNLOCK

        sleep(2); // fumando o cigarro

        sem_wait(&estados_semaphore); // LOCK
        estado_fum[id] = ESPERANDO;

        // FUMANTE acabou o cigarro
        gerar_cena();
        sem_post(&estados_semaphore); //UNLOCK

        // Libera mesa
        sem_post(&mesa_livre_semaphore);
    }
    return NULL;
}

int main() {
    pthread_t thread_agente;
    pthread_t thread_fumantes[N_FUMANTES]; // array com id das threads
    pthread_t thread_combinadores[N_COMBINADORES]; // array com id dos combinadores
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

    // Nenhum ingrediente na mesa inicialmente
    for (i = 0; i < N_FUMANTES; i++) {
        ingredientes_presentes[i] = 0;
    }

    // Semaforos de "campainha" dos pushers, comecam travados: so
    // acordam quando o agente faz o broadcast no fim de cada rodada
    for (i = 0; i < N_COMBINADORES; i++) {
        sem_init(&alerta_combinador_semaphore[i], 0, 0);
    }

    // Cria 3 threads para cada combinador
    for (i = 0; i < N_COMBINADORES; i++) {
        pthread_create(&thread_combinadores[i], NULL, f_combinador, (void *) & combinacoes[i]);
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