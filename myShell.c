#include <stdio.h>
#include "parser.h"
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h> 
//Autor: IVAN CONDE Y LARA SANCHEZ 
//Fecha: 14 DE NOVIEMBRE 2019
//Programa: MyShell
int TRABAJOS_FG_EXE, BG , OutR, InR, ErR;
pid_t* TFG;

//Elementos para realizar jobs

static int vJobs = 0;
static int sizeList = 0;


//MANEJADORES FORE AND BACK
int pidForeground = -1;
void manejadorF()
{
	if (pidForeground != -1) {
		kill(pidForeground, SIGTERM);
	}
}
void manejadorFQUIT()
{
	if (pidForeground != -1) {
		kill(pidForeground, SIGKILL);
	}
}


//Nodo que guarda el proceos
typedef struct Node {
	struct Node* next;
	int value;
	int pidChildren;
	char* estado;
	char line[1024];
} Node;
//Metodo que añade el proceso 
void add_front(Node** head, int value, int pC, char* es, char* l) {
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->value = value;
	new_node->pidChildren = pC;
	new_node->estado = es;
	strcpy(new_node->line, l);


	if (!(*head)) {
		new_node->next = NULL;
		(*head) = new_node;
	}
	else {
		new_node->next = (*head);
		(*head) = new_node;
	}
	printf("1");
	return;
}
//Metodo que añade al final de la lista
Node* addLast(Node** head, int value, int pC, char* es, char* l) {
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->value = value;
	new_node->pidChildren = pC;
	new_node->estado = es;
	strcpy(new_node->line, l);
	Node* auxNode = (*head);
	if ((*head) == NULL) {
		new_node->next = NULL;
		(*head) = new_node;

	}
	else {
		while (auxNode->next != NULL) {
			auxNode = auxNode->next;
		}
		auxNode->next = new_node;
	}
	return new_node;
}
//Metodo que elimina en una posicion exacta
void remove_link(Node** head, int value_to_remove) {
	Node* temp_node = NULL;
	Node* current_node = (*head);
	signal(SIGINT, manejadorF);
	signal(SIGQUIT, manejadorFQUIT);
	if ((*head)->value == value_to_remove) {
		(*head) = (*head)->next;
	}
	else {
		while (current_node->next != NULL) {
			if (current_node->next->value == value_to_remove) {
				temp_node = current_node->next;
				current_node->next = temp_node->next;
				free(temp_node);
			}
			current_node = current_node->next;
		}
	}
	return;
}
//Metodo que elimina al final de la lista
void removeLast(Node** head) {

	Node* auxNode = (*head);

	if ((*head)->next == NULL) {
		(*head) = NULL;
	}
	else {
		while (auxNode->next->next != NULL) {
			auxNode = auxNode->next;
		}
		free(auxNode->next->next);
		auxNode->next->next = NULL;
		auxNode->next = NULL;


	}

}
//Metodo que devuelve el id del proceso del ultimo nodo
int getLastPid(Node** head) {

	Node* auxNode = (*head);
	while (auxNode->next != NULL) {
		auxNode = auxNode->next;
	}
	return auxNode->pidChildren;


}
//Metodo que obtienes un identificador concreto
int getPid(Node** head, int va) {

	Node* auxNode = (*head);
	while (auxNode->next != NULL) {
		if (auxNode->value == va) {
			return auxNode->pidChildren;
		}
		auxNode = auxNode->next;
	}
	return 0;


}
//Metodo para comprobar si algun proceso en background termino
void comprueba(Node** head) {
	Node* auxNode = (*head);
	while (auxNode != NULL) {
		if (waitpid(auxNode->pidChildren, NULL, WNOHANG)) {
			//kill(pidForeground, SIGKILL);		
			remove_link(head, auxNode->value);
			sizeList--;
		}
		auxNode = auxNode->next;

	}

}
//Metodo para imprimir los jobs
void imprimirJobs(Node* head) {
	Node* current_node = head;
	while (current_node) {
		printf("[%d]     %s     %s   \n", current_node->value, current_node->estado,
			current_node->line);
		current_node = current_node->next;
	}

}





void exeN(tline* line, Node* head2, char* buf2)  /*Metodo que ejecutara varios comandos, le pasamos a linea de comandos*/
{
	int c;
	bool b = true;
	for (c = 0; c < line->ncommands; c++) {
		if (access(line->commands[c].filename, X_OK)) {
			b = false;

		}
	}
	if (b == false) {
		printf("Mandato: no se encuentra en el mandato\n");
	}
	else {
		int i, j, HIJO, STATUS, IN, OUT;
		int** pipes;
		pid_t pid;
		IN = dup(0);
		OUT = dup(1);
		HIJO = 0;
		pipes = (int**)malloc((line->ncommands - 1) * sizeof(int*));

		/*Inicializamos el array con los pipes*/
		for (i = 0; i < (line->ncommands - 1); i++) {
			pipes[i] = malloc(2 * sizeof(int));
			pipe(pipes[i]);
		}
		/*Comprobamos si esta en BG o FG*/
		if (!BG) {
			/*Si esta en FG creamos un array de trabajo*/
			TFG = malloc(line->ncommands * sizeof(pid_t));
		}
		dup2(pipes[HIJO][1], 1);
		/*Hacemos todos los mandatos*/
		for (i = 0; i < line->ncommands; i++) {

			/*Creamos un proceso hijo*/
			pid = fork();
			/*Error en la creación del proceso*/
			if (pid < 0) {
				fprintf(stderr, "Fallo en el fork().\n%s\n", strerror(errno));
				exit(1);
				/*Proceso HIJO*/
			}
			else if (pid == 0) {
				if (i == 0) {
					/*Si es el primer hijo, cerramos lo que NO vamos a utilizar*/
					close(pipes[i][0]);
				}
				/*Ejecutamos el mandato*/
				execvp(line->commands[i].argv[0], line->commands[i].argv);
				exit(1);
			}
			else { /*Proceso padre*/
			   /*Descriptor de salida*/
				dup2(OUT, 1);
				/*Comprobamos si se va a ejecutar en BG o en FG*/
				if (!BG) {
					/*Array de FG*/
					TFG[TRABAJOS_FG_EXE] = pid;
					TRABAJOS_FG_EXE++;
				}
				/*Descriptor del proceso hijo*/
				dup2(pipes[HIJO][0], 0);
				/*Cerramos lo que no vayamos a utilziar*/
				close(pipes[HIJO][1]);
				if (i + 1 < line->ncommands - 1) {
					HIJO++;
					dup2(pipes[HIJO][1], 1);
				}
				if (i == line->ncommands - 1 && !BG) { /*ultimo comando a ejecutar*/
					/*Comprobamos si esta en BG*/
					if (line->background) {
						if (sizeList == 0) {
							vJobs = 0;
						}
						sizeList++;
						signal(SIGINT, SIG_IGN);
						signal(SIGQUIT, SIG_IGN);
						vJobs++;
						printf("[%d]  %d \n", vJobs, pid);
						addLast(&head2, vJobs, pid, "Ejecutando", buf2);
						/*Esperamos por los hijos*/
						for (j = 0; j < TRABAJOS_FG_EXE; j++) {
							waitpid(pid, NULL, WNOHANG);
						}
					}
					else {

						for (j = 0; j < TRABAJOS_FG_EXE; j++) {
							/*Esperamos por los hijos*/
							waitpid(TFG[j], &STATUS, 0);
						}
					}
				}
			}
		} /*Cierre del FOR*/
		dup2(IN, 0);
		for (i = 0; i < (line->ncommands - 1); i++)
		{
			/*Liberamos la memoria*/
			free(pipes[i]);
		}
		free(pipes);
		pipes = NULL;
		if (!BG)
		{
			free(TFG);
		}
	}
}




int main(void) {
	char buf[1024];
	tline* line;
	InR = dup(0);
	OutR = dup(1);
	ErR = dup(2);
	Node* head = NULL;

	//Señales que bloquea (CTRL + C) y (CTRL + 4)
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
		//Señales que bloquea (CTRL + C) y (CTRL + 4)
		signal(SIGQUIT, SIG_IGN);
		signal(SIGINT, SIG_IGN);
		line = tokenize(buf);
		if (line == NULL) { /*Si recibo un intro, sigue ejecutando*/
			continue;
		}
		comprueba(&head);

		

		/*RECIBIR UN COMANDO*/
		if (line->ncommands == 1) {
			/* COMANDO CD */
			if (strcmp(line->commands[0].argv[0], "cd") == 0) {
				char* dir;
				if (line->commands[0].argc > 2) {
					printf("cd: demasiados argumentos\n");

				}
				else if (line->commands[0].argc == 2) {
					/* Cambiamos al directorio descrito */
					dir = line->commands[0].argv[1];
					printf("%s", dir);
					/* Errores en CD */
					if (chdir(dir) != 0) {
						printf("Error al cambiar de directorio \n");
					}

				}
				else {
					/* Si no recibe argumentos cambia al directorio HOME */

					dir = getenv("HOME");
					/* Error al cambiar a HOME */
					if (chdir(dir) != 0) {
						printf("Error al cambiar al directorio HOME");
					}
				}

			}
			else if (strcmp(line->commands[0].argv[0], "jobs") == 0) {
				//imprime la lista de procesos en background
				imprimirJobs(head);
			}
			else if (strcmp(line->commands[0].argv[0], "fg") == 0) {
				//if fg no tiene ningun argumento, elimina el ultimo de la lista y lo pone en foreground
				if (line->commands[0].argc == 1) {
					if (sizeList != 0) {
						int pL = getLastPid(&head);
						removeLast(&head);
						sizeList--;
						pidForeground = pL;
						signal(SIGQUIT, manejadorFQUIT);
						signal(SIGINT, manejadorF);
						waitpid(pL, NULL, 0);
					}
					else {
						vJobs = 0;
					}

				}
				else {

					//si fg tiene un argmuento, elimina un proceso concreto y pone ese concreto en foreground
					if (sizeList != 0) {
						int val = atoi(line->commands[0].argv[1]);
						int pL = getPid(&head, val);
						remove_link(&head, val);
						sizeList--;
						pidForeground = pL;
						signal(SIGQUIT, manejadorFQUIT);
						signal(SIGINT, manejadorF);
						waitpid(pL, NULL, 0);
					}
					vJobs = 0;
				}
			}
			else {/* NINGUN COMANDO INTERNO */
			//Comprobamos si ese elemento existe, si no existe manda un mensaje
				if (access(line->commands[0].filename, X_OK)) {
					printf("Mandato: no se encuentra en el mandato\n");
				}
				else {

					int status;
					pid_t pid;
					pid = fork();//Crea un nuevo proceso

					if (pid < 0) {	/* Error en la creacion del hijo */
						fprintf(stderr, "Error: %s\n", strerror(errno));
						exit(-1);	/* Salida por error */

					}
					else if (pid == 0) { /* Ejecución del hijo */

					   /* Redireccion de la entrada estandar */
						if (line->redirect_input != NULL) {
							//Redirecion de entrada al fichero fw, donde es de lectura
							//int fw= open(line->redirect_input, O_CREAT | O_RDONLY | O_TRUNC);
							int fw = open(line->redirect_input, O_CREAT | O_RDONLY);
							if (fw == -1) {
								printf("Error 0\n");
								exit(1);
							}//Cambimos la redirecion 0 (entrada) al archivo fw
							dup2(fw, 0);
							execvp(line->commands[0].filename, line->commands[0].argv);
							printf("Error");//Si no se ejecuta se manda el error

						/* Redireccion de la salida estandar */
						}
						else if (line->redirect_output != NULL) {
							//Redirecion de salida al fichero fw1 de solo lectura, O_TRUNC abre si a existe
							int fw1 = open(line->redirect_output, O_CREAT | O_WRONLY | O_TRUNC);
							if (fw1 == -1) {
								printf("Error 1\n");
								exit(1);
							}//Cambimos la redirecion 0 (salida) al archivo fw1
							dup2(fw1, 1);
							execvp(line->commands[0].filename, line->commands[0].argv);
							printf("Error\n");//Si no se ejecuta se manda el error

						/* Errores en la redireccion de I/O */
						}
						else if (line->redirect_error != NULL) {
							//Redirecion de error al fichero fw2 de solo lectura, O_TRUNC abre si a existe
							int fw2 = open(line->redirect_output, O_CREAT | O_WRONLY | O_TRUNC);
							if (fw2 == -1) {
								printf("Error 2\n");
								exit(1);//Si no se ejecuta se manda el error
							}

							dup2(fw2, 2);//Cambimos la redirecion 2 (error) al archivo fw2
							execvp(line->commands[0].filename, line->commands[0].argv);
							printf("Error");//Si no se ejecuta se manda el error

						}
						else {
							execvp(line->commands[0].filename, line->commands[0].argv);
							printf("Se ha producido un error\n");//Si no se ejecuta se manda el error

						}
					}
					else {
						if (line->background) {
							if (sizeList == 0) {
								vJobs = 0;
							}
							sizeList++;
							//Se ignoran las señales
							signal(SIGINT, SIG_IGN);
							signal(SIGQUIT, SIG_IGN);
							vJobs++;
							printf("[%d]  %d \n", vJobs, pid);
							addLast(&head, vJobs, pid, "Ejecutando", buf);
							waitpid(pid, NULL, WNOHANG);//No bloqueo, continuo


						}
						else {
							pidForeground = pid;
							//SEÑALES AL MANEJADOR
							signal(SIGINT, manejadorF);
							signal(SIGQUIT, manejadorFQUIT);
							waitpid(pid, NULL, 0); //Espera a que el hijo termine, es decir, bloqueo
							if (WIFEXITED(status) != 0) { //Si el hijo ha salido de manera natural es 0, por lo cual no entra
								if (WEXITSTATUS(status) != 0) { //Compruba algo...

								}
							}
						}
					}
				}
			}
		}

		/* MÁS DE UN COMANDO */
		else if (line->ncommands > 1) {

			if (line->redirect_input != NULL) /*Redireccion de entrada*/
			{
				/*tendriamos que comprobar si el fichero existe*/
					freopen(line->redirect_input, "r", stdin); 
			}
			if (line->redirect_output != NULL) { /*Redireccion de salida*/
				freopen(line->redirect_output, "w", stdout); 
			}
			if (line->redirect_error != NULL) { /*Redireccion de error*/
				freopen(line->redirect_error, "w", stderr);
			}
			exeN(line, head, buf);
		}

		/*Restablecemos descriptores*/
		dup2(InR, 0);
		dup2(OutR, 1);
		dup2(ErR, 2);

		printf("msh> ");
	}
	return 0;
}
