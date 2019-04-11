//
// Tiago Fernandes 2017242428
//

#include "drone_movement.h"
#include "header.h"

// Cria o cabeçalho para a lista de encomendas
t_encomenda *cria_encomenda(){
	t_encomenda *lista = (t_encomenda *)malloc(sizeof(t_encomenda));
	if(lista != NULL){
		lista->prox = NULL;
	}
	return lista;
}

// Cria o cabeçalho para a lista dos produtos do sistema
t_produto *cria_produto(){
	t_produto *lista = (t_produto *)malloc(sizeof(t_produto));
	if(lista != NULL){
		lista->prox = NULL;
	}
	return lista;
}

// Cria o ficheiro "log.txt"
void criaLog(){
	FILE *flpt;
	flpt = fopen("log.txt","w");
	if (flpt == NULL){
		printf("Error creating 'log.txt'\n");
		exit(1);
	}
	fclose(flpt);
}

// Escreve no ficheiro "log.txt"
void escreveLog(char msg[]){
	FILE *flp;
	char buffer[80];
	struct tm *getTime;
	time_t now = time(0);
	getTime = gmtime(&now);
	strftime(buffer, sizeof(buffer), "%H:%M:%S", getTime);
	flp = fopen("log.txt","a+");

	if (flp == NULL){
		printf("Error opening 'log.txt'\n");
		exit(1);
	}

	else{
		fprintf(flp, "%s\t%s\n", buffer, msg);
	}

	fclose(flp);
}

// Le o ficheiro "config.txt"
void leFicheiro(){
	FILE *fp;
	char aux[MAX_FGETS], *token;
	int m;

	fp = fopen("config.txt","r");
	
	if (fp == NULL){
		printf("Error opening file!\n");
		exit(1);
	}

	fscanf(fp, "%d, %d\n", &comp_max, &alt_max);
	
	fgets(aux, MAX_FGETS, fp);	
	token = strtok(aux, ", ");
	while (token != NULL){		
		t_produto *novo = (t_produto *) malloc(sizeof(t_produto));

		if (novo == NULL)
			return;

		t_produto *help = cabecalhoP;

		while (help->prox != NULL)
			help = help->prox;


		if (token[strlen(token)-1] == '\n')
			token[strlen(token)-1] = '\0';

		numProdSist++;
		strcpy(novo->nome, token);
		novo->prox = help->prox;
		help->prox = novo;

		token = strtok(NULL, ", ");
	}
	m = numProdSist - 1;

	pSist = malloc(sizeof(t_produto) * numProdSist);
	t_produto *aux_t = cabecalhoP->prox;
	while(aux_t != NULL){
		strcpy(pSist[m].nome, aux_t->nome);
		aux_t = aux_t->prox;
		m--;
	}

	fscanf(fp, "%d\n", &n_drones);

	fscanf(fp, "%lf, %lf, %lf\n\n", &freq, &quant, &tempo);

	fscanf(fp, "%d\n", &n_arm);

	armazens = malloc(sizeof(t_armazem) * n_arm);
	for (int j = 0; j < n_arm; j++){
		int i = 0;

		armazens[j].id_armazem = j;

		fgets(aux, MAX_FGETS, fp);	
		token = strtok(aux, " ");
		strcpy(armazens[j].nome, token);

		token = strtok(NULL, " ");

		token = strtok(NULL, ", ");
		int x = (int )strtol(token, NULL, 10);
		armazens[j].coord_x = x;

		token = strtok(NULL, " ");
		int y = strtol(token, NULL, 10);
		armazens[j].coord_y = y;

		token = strtok(NULL, " ");

		int k = 0;

		while ((token = strtok(NULL, ", "))){
			strcpy(armazens[j].produtos[k].nome, token);

			token = strtok(NULL, ", ");
			int q = strtol(token, NULL, 10);
			armazens[j].produtos[k].quantidade = q;
			i++;
			k++;
		}
		armazens[j].num_produtos = i;
	}
	fclose(fp);
}

// Cria a message queue
void criaMQ(){
    if ((msgID = msgget(IPC_PRIVATE, IPC_CREAT|0777)) < 0){
        perror("Ao criar a message queue");
        exit(1);
    }
}

// Apaga a message queue
void apagaMQ(){
    if (msgctl(msgID,IPC_RMID,NULL) == -1){
        perror("Ao limpar a message queue");
        exit(1);
    }
}

// Cria a shared memory
void criarMemoriaPartilhada(){
	if ((shmidE = shmget(IPC_PRIVATE, sizeof(t_estatisticas), IPC_CREAT | 0766)) != -1){
        #ifdef DEBUG
			printf("\nMemória Partilhada das estatisticas %d criada\n", shmidE);
		#endif
		stats = (t_estatisticas *) shmat(shmidE, NULL, 0);
    	stats->n_encomendas_drone = 0;
		stats->n_produtos_armazem = 0;
		stats->total_encomendas = 0;
		stats->total_produtos = 0;
		stats->tempo_medio = 0.0;
    }
    else
        perror("Erro ao criar memoria partilhada das estatisticas\n");
}

// Cria o named pipe
void criaPipe(){
	unlink(PIPE_NAME);

	// Cria o pipe
	if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600) < 0) && (errno!=EEXIST)){
		perror("Cannot create pipe\n");
		exit(1);
	}

	// Coloca o pipe em modo de leitura
	if ((fd_pipe = open(PIPE_NAME, O_RDWR)) < 0){
		perror("Cannot open pipe for reading\n");
		exit(1);
	}
}

// Apaga o pipe
void apagaPipe(){
	close(fd_pipe);
	unlink(PIPE_NAME);
}

// Inicializa a message queue, a shared memory, o named pipe, o log file e lê o 'config.txt'
void init(){
	//Apaga o ficheiro 'log.txt' existente
	criaLog();

	//Escreve no ficheiro 'log.txt'
	escreveLog("Inicio do programa");

	printf("Criado e escrito no ficheiro 'log.txt'\n\n");
	//Le o ficheiro 'config.txt'
	leFicheiro();
	printf("Ficheiro 'config.txt' lido com sucesso\n\n");

	//Cria memoria partilhada
	criarMemoriaPartilhada();
	printf("Criada memória partilhada com sucesso\n\n");

	//Cria a MSQ
	criaMQ();
	printf("Criada a MSQ com sucesso\n\n");

	//Cria o pipe
	criaPipe();
	printf("Criado o pipe com sucesso\n\n");
}

// Mata o processo armazem
void mataProcesso(){
	printf("Processo armazem fechado com sucesso\n");
	exit(0);
}

// Cria os processos armazens
void criaArmazens(){
	pid_t id[n_arm];
	char mensagem[2048];
	for (int i = 0; i < n_arm; i++){
		if((id[i] = fork()) == 0){
			int id = i;
			signal(SIGINT, mataProcesso);
			printf("[%d] Sou um armazem e estou a espera de receber drones\n", getpid());
			pthread_mutex_lock(&escreve_log);
			sprintf(mensagem, "Criação do processo armazem com número %d", getpid());
			escreveLog(mensagem);
			pthread_mutex_unlock(&escreve_log);
			t_mensagem message;
			while(1){
				if (msgrcv(msgID, &message, sizeof(t_mensagem) - sizeof(long), id + 1, 0) > -1){
					if (message.tipo_mensagem == 0){
						pthread_mutex_lock(&escreve_shm);
						armazens[id].produtos[message.idProduto].quantidade += message.quanty;
						stats->n_produtos_armazem += message.quanty;
						pthread_mutex_unlock(&escreve_shm);
						printf("\n[ENCHIMENTO] Produto %s ganhou +%d no armazem %s\n", armazens[id].produtos[message.idProduto].nome, message.quanty, armazens[id].nome);
						pthread_mutex_lock(&escreve_log);
						sprintf(mensagem, "Armazem %s recebeu +%d do produto %s", armazens[id].nome, message.quanty, armazens[id].produtos[message.idProduto].nome);
						escreveLog(mensagem);
						pthread_mutex_unlock(&escreve_log);
					}	
					else if (message.tipo_mensagem == 1){
						message.confirma = 1;
						msgsnd(msgID, &message, sizeof(t_mensagem) - sizeof(long), 0);
					}
				}
			}
			exit(0);
		}
		else{
			if (id[i] < 0){
				perror("Nao criei filhos");
				exit(1);
			} else{
				#ifdef DEBUG
					printf("[%d] TENHO FILHOS MAS SOU O PROCESSO PAI\n", getpid());
				#endif
			}
		}
		printf("\n");
	}
	printf("\n");
}

// Mostra as estatísticas do programa
void imprimeEstatisticas(){
	pthread_mutex_lock(&escreve_shm);
	printf("\n-------------------------ESTATISTICAS:-------------------------\n");
	printf("->Numero total de encomendas atribuidas a drones: %d\n", stats->n_encomendas_drone);
	printf("->Numero total de produtos carregados de armazens: %d\n", stats->n_produtos_armazem);
	printf("->Numero total de encomendas entregues: %d\n", stats->total_encomendas);
	printf("->Numero total de produtos entregues: %d\n", stats->total_produtos);
	printf("->Tempo medio para a conclusao da encomenda: %.2f\n", stats->tempo_medio);
	printf("---------------------------------------------------------------\n");
	pthread_mutex_unlock(&escreve_shm);
}

// Limpa as threads dos drones
void mataThreads(int aux){
	int i, contador = 0;
	for (i = 0; i < aux; i++){
		if(pthread_join(threads[i], NULL) == 0){
			contador++;
		}
		else
			perror("Um drone nao foi limpo");
	}

	if (contador == n_drones)
		printf("\nTodos os drones foram limpos\n");
	else
		printf("\nNem todos os drones foram limpos\n");
}

// Cria as threads dos drones
void criaThreads(){
	for (int i = 0; i < n_drones; i++){
		create_drone(i);
		if(pthread_create(&threads[i], NULL, trataEncomendas, &avioes[i].id_drone)){
			printf("ERROR CREATING DRONE\n");
		}
	}
}

// Limpa a shared memory
void limpaSHM(){
	//Fazer detach da SHM
	if (shmdt(stats) == -1)
		perror("Erro ao fazer detach da SHM das estatisticas");

	//Limpar a SHM
	if (shmctl(shmidE, IPC_RMID, NULL) == -1)
		perror("Error unmapping shared memory");
}

// Termina com Ctrl + C
void termina(int sig){
	control_c = 1;

	for (int i = 0; i < n_arm + 1; i++)
		wait(NULL);

	imprimeEstatisticas();
	
	limpaSHM();
	printf("\nShared memory limpa com sucesso\n");
	
	apagaPipe();
	printf("\nNamed pipe limpo com sucesso\n");

	apagaMQ();
	printf("\nMessage queue limpa com sucesso\n");

	t_produto *aux_p = cabecalhoP->prox;
	t_produto *antP, *proxP;
	while(aux_p != NULL){
		antP = aux_p;
		proxP = aux_p->prox;
		free(antP);
		aux_p = proxP;
	}
	pthread_mutex_lock(&escreve_log);
	escreveLog("Fim do programa com Ctrl + C");
	pthread_mutex_unlock(&escreve_log);
	kill(0, SIGKILL);
	exit(0);
}

// Após usar o SIGUSR1
void sigusr(int signum){
	imprimeEstatisticas();
}

// Trata da encomenda
t_encomenda *adicionaEncomenda(char nome[], char product[], int quantity, int xDest, int yDest){
	int idOrder = idEncomenda, indice_drone = -1, id_warehouse = -1, id_produto = -1, dist_total = 2 * distance(comp_max, alt_max, 0, 0), final = 0;
	char mensagem[2048];
	for (int i = 0; i < n_arm; i++){
		for (int j = 0; j < armazens[i].num_produtos; j++){
			if (strcmp(product, armazens[i].produtos[j].nome) == 0){
				if (armazens[i].produtos[j].quantidade >= quantity){
					for (int k = 0; k < n_drones; k++){
						double aux_distancia = distance(armazens[i].coord_x, armazens[i].coord_y, avioes[k].x, avioes[k].y) + distance(armazens[i].coord_x, armazens[i].coord_y, xDest, yDest);
						if (avioes[k].estado == 0){
							if (indice_drone == -1 || aux_distancia < dist_total){
								indice_drone = k;
								id_warehouse = i;
								id_produto = j;
								dist_total = aux_distancia;
								final = 1;
								break;
							}
						}
						else if (avioes[k].estado == -2){
							indice_drone = k;
							id_warehouse = i;
							id_produto = j;
							dist_total = aux_distancia;
							final = 1;
							break;
						}
						
					}
					if (indice_drone == -1){
						printf("CTRL + C carregado\n");
						break;
					}
					if (final == 1 || final == 0)
						break;
				}
				else{
					printf("O produto existe no armazem, mas nao ha quantidade suficiente para prosseguir com a encomenda\n");
					final = 0;
					break;
				}
			}
			else{
				printf("O produto nao existe no armazem\n");
				final = 0;
				break;
			}
		}
		if (final == 1 || final == 0)
			break;
	}
	if (final == 1){
		pthread_mutex_lock(&mutex_drones);
		badjoras = indice_drone;
		avioes[indice_drone].estado = 1;
		pthread_cond_broadcast(&notifica_drones);
		pthread_mutex_unlock(&mutex_drones);
		badjoras = -1;
		strcpy(avioes[indice_drone].produtoD, product);
		avioes[indice_drone].quantidadeD = quantity;
		avioes[indice_drone].id_warehouse = id_warehouse;
		avioes[indice_drone].id_produto = id_produto;
		avioes[indice_drone].dist_total = dist_total;
		avioes[indice_drone].x_encomenda_drone = xDest;
		avioes[indice_drone].y_encomenda_drone = yDest;
		strcpy(avioes[indice_drone].nomeD, nome);
		pthread_mutex_lock(&escreve_log);
		sprintf(mensagem, "Encomenda %s enviada ao drone %d", nome, indice_drone);
		escreveLog(mensagem);
		pthread_mutex_unlock(&escreve_log);
		return NULL;
	}
	else{
		pthread_mutex_lock(&escreve_log);
		sprintf(mensagem, "Encomenda %s suspensa por falta de stock", nome);
		escreveLog(mensagem);
		pthread_mutex_unlock(&escreve_log);
		t_encomenda *aux = orders;
		t_encomenda *novo = (t_encomenda *) malloc(sizeof(t_encomenda));
		while (aux->prox != NULL)
			aux = aux->prox;
		strcpy(novo->nome, nome);
		strcpy(novo->produtoE, product);
		novo->idOrder = idOrder;
		novo->quantidadeE = quantity;
		novo->coord_x_dest = xDest;
		novo->coord_y_dest = yDest;
		novo->prox = aux->prox;
		aux->prox = novo;
		return novo;
	}
}

// Drone worker
void *trataEncomendas(void *id_ptr){
	int idD = *((int *) id_ptr);
	printf("[%d] Sou um drone na base (%.2f, %.2f)\n", avioes[idD].id_drone, avioes[idD].x, avioes[idD].y);
	sleep(1);
	char mensagem[2048];
	while (control_c != 1){

		if (avioes[idD].estado == -1 || control_c == 1){
   			printf("[%d] Vou morrer\n", avioes[idD].id_drone);
   			pthread_exit(NULL);
			return NULL;
   		}

		pthread_mutex_lock(&mutex_drones);
		while (badjoras != idD && control_c == 0 && avioes[idD].estado == 0){
			pthread_cond_wait(&notifica_drones, &mutex_drones);
		}
		pthread_mutex_unlock(&mutex_drones);

		while (avioes[idD].estado == 1){
			if (avioes[idD].id_drone == idD){
				double tempo_encomenda = 0;
				printf("\nCoordenadas do drone %d antes de sair para ir ao armazem: (%.2f, %.2f)\n\n", idD, avioes[idD].x, avioes[idD].y);

				if(move_towards(&avioes[idD].x, &avioes[idD].y, armazens[avioes[idD].id_warehouse].coord_x, armazens[avioes[idD].id_warehouse].coord_y) == -1 || move_towards(&avioes[idD].x, &avioes[idD].y, armazens[avioes[idD].id_warehouse].coord_x, armazens[avioes[idD].id_warehouse].coord_y) == -2){
					printf("Correu alguma coisa mal com a encomenda\n");
					char mensagem[1024];
					sprintf(mensagem, "Encomenda %ld falhou ao entregar o produto ao drone!", avioes[idD].idOrder);
					pthread_mutex_lock(&escreve_log);
					escreveLog(mensagem);
					pthread_mutex_unlock(&escreve_log);
					exit(1);
				}

				while (move_towards(&avioes[idD].x, &avioes[idD].y, armazens[avioes[idD].id_warehouse].coord_x, armazens[avioes[idD].id_warehouse].coord_y) > 0){
					tempo_encomenda += tempo;
					printf("Drone %d a mexer-se para o armazem (%.2f, %.2f)\n", avioes[idD].id_drone, avioes[idD].x, avioes[idD].y);
					sleep(tempo);
				}

				printf("\nCoordenadas do drone %d quando chega ao armazem: (%.2f, %.2f)\n\n", avioes[idD].id_drone, avioes[idD].x, avioes[idD].y);
				
				pthread_mutex_lock(&mensagem_drone);
				t_mensagem message;
				message.mtype = avioes[idD].id_warehouse + 1;
				message.coisa = avioes[idD].produtoD;
				message.quanty = avioes[idD].quantidadeD;
				message.tipo_mensagem = 1;

				msgsnd(msgID, &message, sizeof(t_mensagem) - sizeof(long), 0);

				int x = 0;

				while (x == 0){
					if (msgrcv(msgID, &message, sizeof(t_mensagem) - sizeof(long), avioes[idD].id_warehouse + 1, 0) > -1){
						if (message.confirma == 1){
							printf("A receber o produto do armazem...\n");
							sleep(tempo * avioes[idD].quantidadeD);
							x = 1;
						}
					}
				}
				pthread_mutex_unlock(&mensagem_drone);

				pthread_mutex_lock(&escreve_shm);
				stats->n_encomendas_drone++;
				stats->n_produtos_armazem += avioes[idD].quantidadeD;
				stats->total_encomendas++;
				stats->total_produtos += avioes[idD].quantidadeD;
				armazens[avioes[idD].id_warehouse].produtos[avioes[idD].id_produto].quantidade -= avioes[idD].quantidadeD;
				pthread_mutex_unlock(&escreve_shm);

				while (move_towards(&avioes[idD].x, &avioes[idD].y, avioes[idD].x_encomenda_drone, avioes[idD].y_encomenda_drone) > 0){
					tempo_encomenda += tempo;
					printf("Drone %d a mexer-se para o destino (%.2f, %.2f)\n", avioes[idD].id_drone, avioes[idD].x, avioes[idD].y);
					sleep(tempo);
				}

				pthread_mutex_lock(&escreve_shm);
				stats->tempo_medio += tempo_encomenda / stats->total_encomendas;
				pthread_mutex_unlock(&escreve_shm);

				pthread_mutex_lock(&escreve_log);
				sprintf(mensagem, "Encomenda %s entregue no destino", avioes[idD].nomeD);
				escreveLog(mensagem);
				pthread_mutex_unlock(&escreve_log);

				printf("\nCoordenadas do drone %d quando chega ao destino: (%.2f, %.2f)\n\n", avioes[idD].id_drone, avioes[idD].x, avioes[idD].y);

				avioes[idD].estado = -2;

				double casaX = 0, casaY = 0;
				double best_distance_casa = distance(avioes[idD].x, avioes[idD].y, casaX, casaY);
				if(distance(avioes[idD].x, avioes[idD].y, comp_max, 0) < best_distance_casa){
					casaX = comp_max;
					casaY = 0;
					best_distance_casa = distance(avioes[idD].x, avioes[idD].y, comp_max, 0);
				}
				if (distance(avioes[idD].x, avioes[idD].y, 0, alt_max) < best_distance_casa){
					casaX = 0;
					casaY = alt_max;
					best_distance_casa = distance(avioes[idD].x, avioes[idD].y, 0, alt_max);
				}
				if (distance(avioes[idD].x, avioes[idD].y, comp_max, alt_max) < best_distance_casa){
					casaX = comp_max;
					casaY = alt_max;
					best_distance_casa = distance(avioes[idD].x, avioes[idD].y, comp_max, alt_max);
				}

				int ajuda = 0;
				while (move_towards(&avioes[idD].x, &avioes[idD].y, casaX, casaY) > 0){
					printf("Drone %d a voltar para a base mais proxima (%.2f, %.2f)\n", avioes[idD].id_drone, avioes[idD].x, avioes[idD].y);
					sleep(tempo);
					if (avioes[idD].estado == 1){
						ajuda = 1;
						break;
					}
				}

				if (ajuda == 1){
					break;
				}
				else{
					printf("\nCoordenadas do drone %d apos regressar a base: (%.2f, %.2f)\n\n", avioes[idD].id_drone, avioes[idD].x, avioes[idD].y);
					avioes[idD].estado = 0;
					avioes[idD].quantidadeD = -1;
					avioes[idD].id_warehouse = -1;
					avioes[idD].id_produto = -1;
					avioes[idD].dist_total = -1;
					avioes[idD].x_encomenda_drone = -1;
					avioes[idD].y_encomenda_drone = -1;
				}
			}
		}
	}
	pthread_exit(NULL);
	return NULL;
}

// Atualiza as informações do drone
void create_drone(int pos){
	float xDrone, yDrone;
	if (pos % 4 == 0){
		xDrone = 0;
		yDrone = 0;
	}
	else if (pos % 4 == 1){
		xDrone = comp_max;
		yDrone = 0;

	}
	else if (pos % 4 == 2){
		xDrone = comp_max;
		yDrone = alt_max;
	}
	else if (pos % 4 == 3){
		xDrone = 0;
		yDrone = alt_max;
	}
	avioes[pos].x = xDrone;
	avioes[pos].y = yDrone;
	avioes[pos].id_drone = pos;
	avioes[pos].estado = 0;
	idDrone = pos;
}

// Faz a leitura do pipe
void lePipe(){
	int n_chars;
	fd_set read_set;
	char *token;
	char mensagem[2048];
	while(1){
		FD_ZERO(&read_set);
		FD_SET(fd_pipe,&read_set);
		if(select(fd_pipe + 1, &read_set, NULL, NULL, NULL) > 0){
			if((n_chars = read(fd_pipe, str, MAX_FGETS - 1)) > 0){
				str[n_chars-1] = '\0';
				printf("\n%s\n\n", str);
				token = strtok(str, " ");
				if (strcmp(token, "DRONE") == 0){
					token = strtok(NULL, " ");
					if (strcmp(token, "SET") == 0){
						token = strtok(NULL, " ");
						int novo_num = atoi(token);
						if (novo_num >= 1 && novo_num <= MAX_DRONES){
							if (novo_num > n_drones){
								int aux = n_drones; 
								while (aux < novo_num){
									create_drone(aux);
									if (pthread_create(&threads[aux], NULL, trataEncomendas, &avioes[aux].id_drone))
										printf("Erro ao criar um novo drone\n");
									aux++;
								}
								pthread_mutex_lock(&escreve_log);
								n_drones = novo_num;
								sprintf(mensagem, "Número de drones atualizado para %d", n_drones);
								escreveLog(mensagem);
								pthread_mutex_unlock(&escreve_log);
							}
							else if (novo_num < n_drones){
								int aux = novo_num;
								while (aux < n_drones){
									avioes[aux].estado = -1;
									aux++;
								}
								pthread_cond_broadcast(&notifica_drones);			
								for (int i = novo_num; i < n_drones; ++i)
									pthread_join(threads[i], NULL);
								pthread_mutex_lock(&escreve_log);
								n_drones = novo_num;
								sprintf(mensagem, "Número de drones atualizado para %d", n_drones);
								escreveLog(mensagem);
								pthread_mutex_unlock(&escreve_log);
							}
							else if (novo_num == n_drones)
								printf("Esse e o numero atual de drones\n");
						}
						else if (novo_num <= 0 || novo_num > 100){
							pthread_mutex_lock(&escreve_log);
							printf("Tentativa de alterar o número de drones para um número negativo ou inseriu um caracter ou o valor maximo de drones (100) foi ultrapassado\n");
							sprintf(mensagem, "Tentativa de alterar o número de drones para um número negativo ou inseriu um caracter");
							escreveLog(mensagem);
							pthread_mutex_unlock(&escreve_log);
						}
					}		
					else{
						pthread_mutex_lock(&escreve_log);
						printf("Comando invalido\n");
						sprintf(mensagem, "Recebido comando inválido");
						escreveLog(mensagem);
						pthread_mutex_unlock(&escreve_log);
					}
				}
				else if(strcmp(token, "ORDER") == 0){
					token = strtok(NULL, " ");
					char nome[MAX_FGETS];
					strcpy(nome, token);
					token = strtok(NULL, " ");
					if (strcmp(token, "prod:") == 0){
						token = strtok(NULL, ", ");
						int ajuda = 0;
						for (int i = 0; i < sizeof(pSist); i++){
							if (strcmp(pSist[i].nome, token) == 0)
								ajuda++;
						}
						if (ajuda != 0){
							char product[MAX_FGETS];
							strcpy(product, token);
							token = strtok(NULL, " ");
							int quantity = atoi(token);
							if (quantity > 0){
								token = strtok(NULL, " ");
								if (strcmp(token, "to:") == 0){
									token = strtok(NULL, ", ");
									int destinoX = atoi(token);
									if (destinoX >= 0 && destinoX <= comp_max){
										token = strtok(NULL, " ");
										int destinoY = atoi(token);
										if (destinoY >= 0 && destinoY <= alt_max){
											idEncomenda++;
											pthread_mutex_lock(&escreve_orders);
											pthread_mutex_lock(&escreve_log);
											sprintf(mensagem, "Encomenda %s recebida pela Central", nome);
											escreveLog(mensagem);
											pthread_mutex_unlock(&escreve_log);
											adicionaEncomenda(nome, product, quantity, destinoX, destinoY);
											pthread_mutex_unlock(&escreve_orders);
										}
										else{
											pthread_mutex_lock(&escreve_log);
											printf("A coordenada Y da encomenda nao pode ser negativa e tem de estar dentro do espaco\n");
											sprintf(mensagem, "Recebido comando inválido");
											escreveLog(mensagem);
											pthread_mutex_unlock(&escreve_log);
										}
									}
									else{
										pthread_mutex_lock(&escreve_log);
										printf("A coordenada X da encomenda nao pode ser negativa e tem de estar dentro do espaco\n");
										sprintf(mensagem, "Recebido comando inválido");
										escreveLog(mensagem);
										pthread_mutex_unlock(&escreve_log);
									}
								}
								else{
									pthread_mutex_lock(&escreve_log);
									printf("Comando invalido\n");
									sprintf(mensagem, "Recebido comando inválido");
									escreveLog(mensagem);
									pthread_mutex_unlock(&escreve_log);
								}
							}
							else{
								pthread_mutex_lock(&escreve_log);
								printf("A quantidade de produtos da encomenda nao pode ser negativa e tem de ser um numero\n");
								sprintf(mensagem, "Recebido comando inválido");
								escreveLog(mensagem);
								pthread_mutex_unlock(&escreve_log);
							}
						}
						else{
							pthread_mutex_lock(&escreve_log);
							printf("O produto nao existe no programa\n");
							sprintf(mensagem, "Recebido comando inválido");
							escreveLog(mensagem);
							pthread_mutex_unlock(&escreve_log);
						}
					}	
					else{
						pthread_mutex_lock(&escreve_log);
						printf("Comando invalido\n");
						sprintf(mensagem, "Recebido comando inválido");
						escreveLog(mensagem);
						pthread_mutex_unlock(&escreve_log);
					}
				}
				else{
					pthread_mutex_lock(&escreve_log);
					printf("Comando invalido\n");
					sprintf(mensagem, "Recebido comando inválido");
					escreveLog(mensagem);
					pthread_mutex_unlock(&escreve_log);
				}
			}
		}
		else if(control_c == 1)
			exit(0);
	}
}

// Cria o processo principal
void processoCentral(){
	printf("[%d] Sou o processo Central\n", getpid());
	criaThreads();
	lePipe();
}

// Acaba com o processo Central
void mataProcessoCentral(){
	control_c = 1;
	pthread_cond_broadcast(&notifica_drones);
	mataThreads(n_drones);
	printf("Processo Central fechado com sucesso\n");
	exit(0);
}

int main(int argc, char const *argv[]){
	cabecalhoP = cria_produto();
	orders = cria_encomenda();
	int conta = 0;
	time_t t;
	srand((unsigned) time(&t));
	init();
	criaArmazens();
	signal(SIGINT, termina);
	signal(SIGUSR1, sigusr);
	sleep(1);
	if (fork() == 0){
		signal(SIGINT, mataProcessoCentral);
		processoCentral();
		exit(0);
	}
	sleep(3);
	while(1){
		t_mensagem message;
		pthread_mutex_lock(&mensagem_drone);
		int rand_p = rand() % armazens[conta % n_arm].num_produtos;
		message.mtype = (conta % n_arm) + 1;
		message.idProduto = rand_p;
		message.quanty = quant;
		message.tipo_mensagem = 0;
		msgsnd(msgID, &message, sizeof(t_mensagem) - sizeof(long), 0);
		pthread_mutex_unlock(&mensagem_drone);
		sleep(freq * tempo);
		conta++;
	}
	escreveLog("Fim do programa");
	termina(0);
	return 0;
}
