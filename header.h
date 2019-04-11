//
// Tiago Fernandes 2017242428
//

#ifndef HEAD
#define HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <mqueue.h>
#include <sys/msg.h>

#define MAX_FGETS 200
#define PIPE_NAME "pipe"
#define MAX_DRONES 100

// Varíavel para o named pipe
int fd_pipe;

// Variável para receber strings (caso seja necessário)
char str[2048];

// Variável para o CTRL + C
int control_c = 0;
int numProdSist = 0;

//Variavel para a SHM
int shmidE;

//Variável para a MSQ
int msgID;
int atualizou = 0;

// Mutexes
pthread_mutex_t escreve_shm = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t escreve_log = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t escreve_orders = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mensagem_drone = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mensagem_stocks = PTHREAD_MUTEX_INITIALIZER;

// Mutex e variavel de condi�ao associados
pthread_mutex_t mutex_drones = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t notifica_drones = PTHREAD_COND_INITIALIZER;
int badjoras = -1;

//Variaveis globais para as coisas que vêm no config.txt
int comp_max;
int alt_max;
char **prod;
int n_drones;
double freq;
double quant;
double tempo;
int n_arm;

//Colocar drones nos spots
int xDrone;
int yDrone;

//Arrays das threads e contador para os id's
pthread_t threads[MAX_DRONES];
int idDrone;
long idEncomenda = 1;
int contador_encomendas = 0;

typedef struct prodQuant{
	char nome[MAX_FGETS];
	int quantidade;
}t_prod_quant;

t_prod_quant *cabecalhoPQ;

typedef struct prodSist{
	char nome[MAX_FGETS];
	struct prodSist *prox;
}t_produto;

t_produto *cabecalhoP;
t_produto *pSist;

typedef struct armazem{
	char nome[MAX_FGETS];
	int id_armazem;
	int coord_x;
	int coord_y;
	t_prod_quant produtos[3];
	int num_produtos;
}t_armazem;

t_armazem *armazens;

typedef struct encomenda{
	char nome[MAX_FGETS];
	char produtoE[MAX_FGETS];
	long idOrder;
	int quantidadeE;
	int coord_x_dest;
	int coord_y_dest;
	struct encomenda *prox;
}t_encomenda;

t_encomenda *orders;

typedef struct estatisticas{
	int n_encomendas_drone;
	int n_produtos_armazem;	// recebidos pelos armazens
	int total_encomendas;
	int total_produtos;	// pelos drones
	double tempo_medio;
}t_estatisticas;

t_estatisticas *stats;

typedef struct drone{
	double x;
	double y;
	int id_drone;
	char nomeD[MAX_FGETS];
	char produtoD[MAX_FGETS];
	long idOrder;
	int quantidadeD;
	int x_encomenda_drone;
	int y_encomenda_drone;
	int id_warehouse;
	int id_produto;
	int id_aviao;
	double dist_total;
	int estado;		// (0) - Desocupado <-> (1) - Ocupado <-> (-1) - CTRL + C carregado <-> (-2) - Acabou de entregar e esta a voltar para a base
	struct drone *prox;
}t_drone;

t_drone avioes[MAX_DRONES];

typedef struct mensagem{
	long mtype;
	char *coisa;
	int quanty;
	int tipo_mensagem;
	int idProduto;
	int confirma;
}t_mensagem;

t_encomenda *cria_encomenda();
t_produto *cria_produto();
void criaLog();
void escreveLog(char msg[]);
void leFicheiro();
void criaMQ();
void apagaMQ();
void criarMemoriaPartilhada();
void criaPipe();
void apagaPipe();
void init();
void mataProcesso();
void criaArmazens();
void imprimeEstatisticas();
void mataThreads(int aux);
void criaThreads();
void limpaSHM();
void termina(int sig);
void sigusr(int signum);
t_encomenda *adicionaEncomenda(char nome[], char product[], int quantity, int xDest, int yDest);
void *trataEncomendas(void *id_ptr);
void create_drone(int pos);
void lePipe();
void processoCentral();
void mataProcessoCentral();

#endif