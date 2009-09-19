#if defined(__unix__) || defined(__VMS)
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <winsock.h>
#include <io.h>
#include "getopt.h"
#endif
#if defined(VAX)
#include "getopt.h"
#endif
#include "ftplib.h"
#include "unistd.h"

#if !defined(S_ISDIR)
#define S_ISDIR(m) ((m&S_IFMT) == S_IFDIR)
#endif

/* exit values */
#define EX_SYNTAX 1 	/* command syntax errors */

#define FTP_GET 1		/* retreive files */
#define FTP_DIR 2		/* verbose directory */
#define FTP_LIST 3		/* terse directory */

#define EXIT_ON_ERROR 1 /*Si a 1, toutes le erreurs font quitter le prog (RECOMMANDE) sinon 0*/
#define DEBUG  0 /*Ligne de debugage principal si a 1*/
#define DEBUG_RECV  0 /*Ligne de debuggage sur les receive COM_D si a 1*/
#define BUFSIZE 8192 /*Taille du buffer*/

static netbuf *conn = NULL;
static int logged_in = 0;
static char mode = 'I';

// Déclarer une structure qui contient les informations de la connexion courante
static struct session {
	char *host; /*adresse ip ou nom du serveur*/
	char *user; /*nom d'utilisateur pour faire login*/
	char *pass; /*le password pour faire login*/
	int port; 	/*Port de communication (i.e 21 ou 20*/
} current;

int open_connexion(void) {
	if (conn){ //Si on a connecté à le serveur, affichier le message
		fprintf(stderr,"Already connected, use close first\n%s", FtpLastResponse(conn));
		return -1;
	}

	if (current.host == NULL) {
		fprintf(stderr,"Host name not specified\n");
		exit(EX_SYNTAX);
	}
	if (!logged_in) {
		//on appelle le function FtpConnect et dertemine la connexion
		if (!FtpConnect(current.host, &conn)) {
			fprintf(stderr,"Unable to connect to node %s\n%s", current.host,
					ftplib_lastresp);
			return -1;
		}
	}
	return 1;
}

int login(void) {
	//on appelle le function FtpLogin sur la connexion TCP avec le nom et pass de client
	if (!FtpLogin(current.user, current.pass, conn)) {
		fprintf(stderr,"Login Incorrect \n", FtpLastResponse(conn));
		conn = NULL;
		return -1;
	}
	logged_in++;
	return 1;
}

// Lire les données du argument fd de la ligne de commande
int wait_for_arg(int fd) {
	fd_set rfds;
	struct timeval tv;
	int retval;

	/* Surveiller stdin (fd 0) en attente d'entrés */
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	/* Pendant 5 secondes maxi */
	tv.tv_sec = 0;
	tv.tv_usec = 300;
	retval = select(fd + 1, &rfds, NULL, NULL, &tv);
	/* Retourner des résultats */
	if (retval>0)
	return 1; /*Des donnéss sont lues */
	else
	return -1;/*Pas de donnés lues*/
}

/*fonction verifiant les arguments du main*/
void valid_param(int argc, char** argv) {
	int i;
	// Si la ligne de commande n'est pas valide --> exit
	if (argc < 2) {
		printf("Usage : %s serveur_ftp [port] \n", argv[0]);
		if (EXIT_ON_ERROR) exit(argc);
	}
	// Lire les arguments dans la ligne de commande
	for (i = 1; i < argc; i++) {
		/*serveur sur lequel se connecter*/
		if (i == 1) {
			strcpy(current.host, argv[i]);
			printf("ip = %s", current.host);
			printf(" arv = %s", argv[i]);
		}
		/*port du ftp*/
		if (i == 2) {
			current.port = atoi(argv[i]);
		}
	}
	// Si l'utilisateur n'entre pas le port pour ftp --> le port par défaut est 21
	if (current.port == 0) {
		current.port = 21;
		printf("Port par default : 21\n");
	}
}

/*Affiche l'aide des commandes clients*/
void aide() {
	printf("Les commandes FTP sont les suivantes :\n");
	printf("------------------------------------------------------------\n");
	printf(" HELP 	(AIDE)  Afficher cette page\n");
	printf(" OPEN           Ouvrir une nouvelle connexion\n");
	printf(" CLOSE          Fermer connexion sans quitter ftp\n");
	printf(" QUIT 	        Quitter le ftp\n");
	printf(" CWD  	(CD)    Changer de répertoire\n");
	printf(" LS   	(DIR)   Afficher la liste des fichiers distants\n");
	printf(" GET            Récuperer un fichier depuis le serveur\n");
	printf(" PUT            Envoyer un fichier sur le serveur\n");
	printf("------------------------------------------------------------\n");
}

int main(int argc, char** argv) {
	int err;
	char *cmd;
	char *arg;
	char fnm[256];

	// déclarer le mémoire
	cmd = (char*) malloc(254 * sizeof(char));
	arg = (char*) malloc(254 * sizeof(char));
	current.host = (char*) malloc(254 * sizeof(char));
	current.user = (char*) malloc(254 * sizeof(char));
	current.pass = (char*) malloc(254 * sizeof(char));

	/*vérifier et lire les arguments de la ligne de commande*/
	if (argc > 1) {
		valid_param(argc, argv);// vérifier les parametres
		open_connexion(); // créer le connextion TCP
	}

	do {
		printf("/ftp> ");

		/*On lire une nouvelle commande */
		scanf("%s", cmd);

		/*le cas d'aide*/
		if ((strcasecmp(cmd, "AIDE") == 0) || (strcasecmp(cmd, "HELP") == 0)) {
			aide();
		}

		/*le cas de la creation de connexion*/
		// open [nom,IP] [port]
		else if (strcasecmp(cmd, "OPEN") == 0) {
			int port = 21;
			// Lire le serveur à connecter
			if (wait_for_arg(0) > 0) {
				scanf("%s", arg);
			} else if(!conn){
				printf("Serveur distant : ");
				scanf("%s", arg);
			//Lire le port à connecter
				if (wait_for_arg(1) > 0) {
					scanf("%d", &port);
				} else {
					printf("Port : ");
					scanf("%d", &port);
				port = 0;
				}
				strcpy(current.host, arg);
				current.port = port;
			}

			// open la connexion
			if (open_connexion() != -1) {
				// Lire le nom de client de login
				if (wait_for_arg(2) > 0) {
					scanf("%s", current.user);
				} else {
					printf("Nom : ");
					scanf("%s", current.user);
				}
				// Lire le pass de login
				if (wait_for_arg(3) > 0) {
					scanf("%s", current.pass);
				} else {
					current.pass = getpass("Password: ");
				}
				login();
			}
		}

		// le cas de sort completement de la session de FTP
		else if (strcasecmp(cmd, "QUIT") == 0) {
			if (conn){
				FtpQuit(conn);
				exit(0);
			}
			exit(0);
		}
		// le cas de ferme de la  session ftp
		else if (strcasecmp(cmd, "CLOSE") == 0) {
			if (conn){
				FtpClose(conn);
				conn = NULL;
				logged_in = 0;
			}
		}

		// le cas d'accéder un dossier
		else if ((strcasecmp(cmd, "CWD") == 0) || (strcasecmp(cmd, "CD") == 0)) {
			if (wait_for_arg(0) > 0) {
				scanf("%s", arg);
			} else {
				printf("fichier distant : ");
				scanf("%s", arg);
			}
			strcpy(fnm, arg);
			if (conn){
				err = FtpChdir(fnm, conn);
			}
			else printf("No connexion! \n");
		}

		// le cas d'affiche la liste de dossiers et fichiers
		else if ((strcasecmp(cmd, "LIST") == 0) || (strcasecmp(cmd, "LS") == 0)
				|| (strcasecmp(cmd, "DIR") == 0)) {
			if (wait_for_arg(0) > 0) {
				scanf("%s", arg);
				strcpy(fnm, arg);
			} else {

				strcpy(fnm, ".\n");
			}
			if (conn){
				err = FtpDir(NULL, fnm, conn);
			}
			else printf("No connexion! \n");
		}

		// commande GET
		else if ((strcasecmp(cmd, "GET") == 0)) {
			if (wait_for_arg(0) > 0) { // nom du fichier à distant
				scanf("%s", arg);
			} else {
				printf("Fichier distant : ");
				scanf("%s", arg);
				printf("%s\n", arg);
			}
			char* local;
			local = (char*) malloc(254 * sizeof(char));
			if (wait_for_arg(1) > 0) { // nom du fichier local
				scanf("%s", local);
			} else {
				printf("Fichier local : ");
				scanf("%s", local);
				if (strcmp(local, "") == 0)
					strcpy(local, arg);
				printf("%s\n", local);
			}
			// obtenir un fichier à partir du répertoire distant
			if (conn){
				err = FtpGet(local, arg, mode, conn);
				if (ftplib_debug && !err)
					printf("%s retrieved\n", local);
			}
			else printf("No connexion! \n");
		}

		// commande PUT
		else if ((strcasecmp(cmd, "PUT") == 0)) {
			if (wait_for_arg(0) > 0) { // nom du fichier local
				scanf("%s", arg);
			} else {
				printf("Fichier local : ");
				scanf("%s", arg);
				printf("%s\n", arg);
			}
			char* distant;
			distant = (char*) malloc(254 * sizeof(char));

			if (wait_for_arg(1) > 0) { // nom du fichier à distant
				scanf("%s", distant);
			} else {
				printf("Fichier distant : ");
				scanf("%s", distant);
				if (strcmp(distant, "") == 0)
					strcpy(distant, arg);
				printf("%s\n", distant);
			}
			// stocker un fichier dans le répertoire distant courant
			if (conn){
				err = FtpPut(arg, distant, mode, conn);
			}
			else printf("No connexion! \n");
		}

		// la reste...
		else {
			printf("?Commande non valide.\n");
		}

	} while (1);

	return 1;
}
