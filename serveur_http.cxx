#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BACKLOG 128
#define NB_CLIENTS 1000
#define TAILLE_BUFFER 5 // une petite valeur juste pour les test.
#define RequeteMax 5
#define nom_serveur "Mon serveur WEB"

using namespace std;

void exitErreur(const char * msg) {
    perror(msg);
    exit( EXIT_FAILURE);

}
class SocketReader{
private:
    int socketfd;
    char msgRead[TAILLE_BUFFER];
    int  start ; //la position du premier élément de l'intervalle de recherche de \n
    int count; // la taille de l'intervalle de recherhce de \n

public :
    SocketReader(int socketfd){
        count = 0;
        this->socketfd = socketfd;
    }

    int readLine(int sockfd, string & line)
    {
        //line : la ligne lue sans \n
        line = ""; int i ;
        while(true) {
            if (count ==0){
                count = read(sockfd, msgRead, sizeof(msgRead));
                if (count == -1) return -1; // cas d'erreur
                if (count == 0) return 0; // cas de deconnexion
                start = 0 ;
            }
            for ( i=start; i<count; i++)
                if (msgRead[i] == '\n') break;
            if(i<count) { // on a trouvé /n
                line = line + string(msgRead, start,(i-1)-start + 1); // rappel : nb élements dans une suite = l'indice du dernier - l'indice du premier +1
                if (i==count-1) // \n est le dernier élement
                    count = 0; // la prochaine fois il faut lire depuis la socket
                else start =i+1; // il reste encore des élements non traites dans msgRead => le premier élement du prochain readLine() est i+1
                break ;}
            else {
                // on n'a pas trouvé le \n
                line = line + string(msgRead,start, (i-1)-start+1); // l'indice du dernier élement à considérer est i-1 (l'element i est en dehors de l'intervalle)
                count = 0 ; // la prochaine fois il faut lire depuis la socket
            }
        }
        return line.size();
    }

};

int main(int argc, char * argv[]) {

    if(argc!=2) exitErreur("usage invalide : port manquant");

    int sock_serveur = socket(AF_INET, SOCK_STREAM, 0);


    struct sockaddr_in sockaddr_serveur;

    sockaddr_serveur.sin_family = AF_INET;
    sockaddr_serveur.sin_port = htons(atoi(argv[1]));
    sockaddr_serveur.sin_addr.s_addr = htonl(INADDR_ANY);

    int yes = 1;
    if (setsockopt(sock_serveur, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
            == -1)
        exitErreur("setsockopt");

    if (bind(sock_serveur, (struct sockaddr *) &sockaddr_serveur,
             sizeof(sockaddr_in)) == -1)
        exitErreur("bind");

    if (listen(sock_serveur, BACKLOG) == -1)
        exitErreur("listen");

    int sock_client;


    string msgRecu("");

    cout << "Serveur lancé" << endl;

    for (int i = 1; i <= NB_CLIENTS; i++) {

        // Le serveur attend un client
        sock_client = accept(sock_serveur, NULL, NULL);
        if (sock_client == -1)
            exitErreur("accept");
        int p = fork();

        if(!p){// processus fils
            close(sock_serveur);
            for(int i = 1; i<=RequeteMax; ++i){
                SocketReader sr(sock_client);
                int n;
                string ligne;
                string file_name = "";
                while(true) {
                    n = sr.readLine(sock_client,ligne);
                    // client s'est déconnecté
                    if (!n ) break ;
                    // cas d'erreur
                    if (n==-1) break;

                    cout << ligne << endl;
                    if (file_name =="") file_name = ligne.substr(4,ligne.size()-4-9);
                    cout << file_name << endl;
                    if (ligne == "" || ligne=="\r") break;

                }

                if(n==0 || n==-1) break;
                int fichier = open(file_name.c_str(), O_RDWR);
                if(fichier== -1){
                    string message_erreur = "<html><body><h1> URL Introuvable </h1></body></html>";
                    string reponse_erreur = "HTTP/1.1 Introuvable \r\nServeur : Mon Serveur Web" + to_string(message_erreur.length())+"\r\n\r\n"+message_erreur+"\r\n";
                    write(sock_client, reponse_erreur.c_str(), reponse_erreur.size());
                }
                else{
                    struct stat s;
                    fstat(fichier,&s);     
                    int taille_fichier = s.st_size;    
                    char tampon[taille_fichier];
                    
                    int l;
                    while((l = read(fichier,tampon,sizeof(tampon))) !=0){
                        write(sock_client,tampon,l);
                    }
                    
                }
            }
            close(sock_client);
            exit(EXIT_SUCCESS);
        }
        //ici c'est le père
        close(sock_client);

    }
    close(sock_serveur);
    return EXIT_SUCCESS;   
}
    
    
    
