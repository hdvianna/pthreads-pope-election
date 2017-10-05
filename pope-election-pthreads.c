#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_THREADS 7 /*Máximo de treads de cardeais a serem criadas*/

typedef struct s_cardeal {
	pthread_t id_cardeal;/*id da thread do cardeal*/
	int total_votos;/*total de votos q o cardeal recebeu em um turno de votação*/
} t_cardeal;
t_cardeal v_cardeal[MAX_THREADS]; /*Vetor q armazena dados referentes aos cardeais*/

typedef struct s_mais_votado {
	int idx;
	int total_votos;
} t_mais_votado;

sem_t usa_urna; /*Variável de semáforo q decide se o cardeal poderá usar a urna*/
sem_t sem_fim; /*Variável de semáforo q define se haverá mais um turno de votação*/
sem_t cumprimento; /*Variável q controla o acesso aos cumprimentos*/
sem_t tem_cumprimento; /*Variável q controla o acesso aos cumprimentos*/
sem_t ntem_cumprimento; /*Variável q controla o acesso aos cumprimentos*/

pthread_t id_coroinha;/*id da thread do coroinha*/

int urna[MAX_THREADS]; /*Vetor q armazena o voto dos cardeais*/

int total_votos; /*Armazena a quantidade de cardeais q já votaram*/

int fim_eleicao = 0;/*Sinal q define o fim da eleição*/

int eleito; /*Indice do cardeal eleito*/
int cardeal_cumprimento; /*Indice do cardeal q estah cumprimentando*/
int posso_cumprimentar;/*Variavel q indica se eh possivel cumprimentar o papa*/
int receb_cumprimento = 0;/*Variavel q indica se eh possivel cumprimentar o papa*/

/*Thread q simula o comportamento do cardeal*/
void *cardeal(void *idx) {
	int i = *(int*) idx;/*Transforma o argumento enviado como ponteiro para um tipo integer*/
	int j;

	printf ("Oi vaticano! Eu sou o cardeal %d! thread_id: %i\n", 
			i, (int) pthread_self());	
	while (!fim_eleicao) {		
		printf("O cardeal %i estah votando.\n", i);		
		sleep(numero_aleatorio(0,10));/*Aguarda o cardeal decidir seu voto*/
	
		/*Semaforo q faz com q o cardeal aguarde a sua vez para	utilizar a urna*/
		sem_wait(&usa_urna);
		v_cardeal[i].total_votos = 0;
		printf("O cardeal %i estah usando a urna\n", i);
		urna[i] = numero_aleatorio(0, MAX_THREADS-1);/*Gera um voto para o cardeal*/
		//urna[i] = numero_aleatorio(1, 1);/*Gera um voto para o cardeal*/
		printf("O voto do cardeal %i foi: %i\n", i, urna[i]);
		total_votos++;/*incrementa a quantidade de cardeais q votara*/
		sem_post(&usa_urna);

		sem_wait(&sem_fim);/*Aguarda a apuração dos votos*/
	}

	/*Cumprimenta o papa, ou rebe o cumprimento*/
	if (i == eleito) { /*Eh o papa, espera ser cumprimentado*/
		j = 0;
		while (j < (MAX_THREADS - 1)) {
			while(!receb_cumprimento) 
				sem_wait(&ntem_cumprimento);
			sem_wait(&cumprimento);			
			printf("Estou recebendo o cumprimento de: %i\n", cardeal_cumprimento);
			sleep(numero_aleatorio(0,10));
			receb_cumprimento = 0;
			sem_post(&tem_cumprimento);
			sem_post(&cumprimento);
			j++;
		}	
	} else {
		while(receb_cumprimento)
			sem_wait(&tem_cumprimento);

		sem_wait(&cumprimento);
		receb_cumprimento = -1;
		cardeal_cumprimento = i;
				
		sem_post(&ntem_cumprimento);
		sem_post(&cumprimento);		
	}
	
}

/*Thread q simula o comportamento do coroinha*/
void *coroinha() {
	int i; 
	t_mais_votado primeiro, segundo;
	
	primeiro.total_votos = 0;
	segundo.total_votos = 0;

	while (!fim_eleicao) {
		/*Aguarda até q todos os cardeais votem*/	
		while (total_votos < MAX_THREADS)
			sleep(numero_aleatorio(0,10));	
	
		printf("O coroinha estah apurando os votos\n", i);
		sleep(numero_aleatorio(0,10));
		for (i=0; i < MAX_THREADS; i++) {
			sem_wait(&usa_urna);
			v_cardeal[urna[i]].total_votos++; /*Incrementa o acumulador de votos do cardeal*/			
			/*Verifica se o cardeal atual tem mais votos q o primeiro*/
			if (v_cardeal[urna[i]].total_votos > primeiro.total_votos) {
				primeiro.idx = urna[i];
				primeiro.total_votos = v_cardeal[urna[i]].total_votos;
			} else {
				/*Verifica se o cardeal atual tem mais votos q o segundo*/
				if (v_cardeal[urna[i]].total_votos > segundo.total_votos) {
					segundo.idx = urna[i];
					segundo.total_votos = v_cardeal[urna[i]].total_votos;
				}
			}
			urna[i] = 0;/*Limpa o voto da urna*/
			sem_post(&usa_urna);		
		} 

		if (primeiro.total_votos > segundo.total_votos) {
			printf("HABEMOS PAPA\nO papa eleito eh o cardeal %i com %i votos!\n", primeiro.idx, primeiro.total_votos);
			eleito = primeiro.idx;/*Atribui o papa eleito*/
			fim_eleicao = -1;
		} else  {						
			total_votos = 0;
			primeiro.total_votos = 0;
			primeiro.idx = -1;
			segundo.total_votos = 0;
			segundo.idx = -1;
			printf("Houve empate!\n");					
		}
		/*Manda um sinal para os cardeais q estavam aguardando*/
		for(i=0; i < MAX_THREADS; i++)
			sem_post(&sem_fim); 
	}
}

int numero_aleatorio(int min, int max) {	
	return min + rand()%((max-min)+1);
}

/*Função principal do programa*/
int main() {
	int i =0, *idx;	

	srand(time (NULL));/*Cria o seed para gerar números aleatórios*/	
	
	sem_init(&usa_urna, 0, 1);/*Inicia o semáforo*/
	sem_init(&sem_fim, 0, 0);/*Inicia o semáforo*/
	sem_init(&cumprimento, 0, 1);/*Inicia o semáforo*/
	sem_init(&tem_cumprimento, 0, 1);/*Inicia o semáforo*/
	sem_init(&ntem_cumprimento, 0, 1);/*Inicia o semáforo*/

	/*Cria <MAX_THREADS> threads de cardeal*/	
	for (i=0;i < MAX_THREADS; i++) {		
		idx = (int*) malloc(sizeof(int));
		*idx = i;
		pthread_create(&v_cardeal[i].id_cardeal, NULL, 
						cardeal, (void*) idx);
	}
	pthread_create(&id_coroinha, NULL, coroinha, NULL);	
	/*Espera as threds dos cardeais finalizarem*/
	for (i=0; i < MAX_THREADS; i++)
		pthread_join(v_cardeal[i].id_cardeal, NULL);
	
	pthread_join(id_coroinha, NULL);
	
}

