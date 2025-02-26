#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>




const char* default_ip_start = "192.168.1.1";
const char* default_ip_end = "192.168.255.255";

#define default_port_start 1
#define default_port_end 1000
#define default_no_threads 10


pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *outf=NULL;

typedef struct { 
    struct in_addr start;
    struct in_addr end;
    int start_port;
    int end_port;
} IPData;



typedef struct { 
    char *hostname;
    int start_port;
    int end_port;
} portData;



// scan_ports
void *scan_ports(void *arg) { 
    portData *data = (portData*) arg;
    struct sockaddr_in sa;
    int sock, err, i;
    char hostname[100];


    strncpy(hostname, data->hostname, sizeof(hostname) - 1);
    sa.sin_family = AF_INET;

    if (isdigit(hostname[0])) { 
        sa.sin_addr.s_addr = inet_addr(hostname);
    }
    

    for (i = data->start_port; i <= data->end_port; i++) { 
        sa.sin_port = htons(i);
        sock = socket(AF_INET, SOCK_STREAM, 0);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            perror("setsockopt failed\n");

        if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
            perror("setsockopt failed\n");



        if (sock < 0) { 
            pthread_exit(NULL);
        }

        err = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
        if (err == 0) { 


            fprintf(outf, "Portul %i e deschis pe: %s \n", i, hostname);




        } else { 
            printf("Portul %i nu e deschis pe: %s\n", i, hostname);
        }
        close(sock);
    }
    pthread_exit(NULL);
}


// is_ip_active
int is_ip_active(const char* ip_address) { 
    int status;

    pid_t pid;
    if((pid = fork())==-1) { 
        perror("fork err");
        exit(-7);
    }
        if (pid == 0) { 
        // fiu
        char command[200];
        snprintf(command, sizeof(command), "ping -c 1 -W 0.1 %s > /dev/null 2>&1", ip_address);
        
        if(execlp("sh", "sh", "-c", command, (char*)NULL)==-1) { 
            perror("execlp err\n");
            exit(1);  
        }
    } else { 
        // parinte
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    return 0;
}


// get_mac_address
#define bufferSize 1000

void get_mac_address(const char* ip_address) { 
    FILE *fp;
    char output[bufferSize];
    char mac[bufferSize];
    size_t index = 0;
    

    char command[bufferSize];
    snprintf(command, sizeof(command), "arp -a %s", ip_address);

    fp = popen(command, "r");
    if (fp == NULL) { 
        perror("err popen");
        return;
    }

    while (fgets(output + index, bufferSize - index, fp) != NULL) { 
        index += strlen(output + index);
        if (index >= bufferSize - 1) { 
            break;
        }
    }

    pclose(fp);

    char *token;
    int c = 0;

    token = strtok(output, " \n");
    while (token != NULL) { 
        c++;
        if (c == 4) { 
            
            if (strlen(token) == 17 && token[2] == ':' && token[5] == ':' && token[8] == ':' &&
                token[11] == ':' && token[14] == ':') { 
                strcpy(mac, token);

                
                fprintf(outf, "Adresa MAC pt IP %s: %s\n", ip_address, mac);

                
            } else { 
                fprintf(outf, "Nu s-a gasit adrsa valida pt %s\n", ip_address);
            }
            break;
        }
        token = strtok(NULL, " \n");
    }
}










// scan_ip_range
void* scan_ip_range(void* arg) { 
    int r = 0;
    IPData* data = (IPData*)arg;
    struct in_addr current = data->start;
    pthread_t port_thread;
    while (ntohl(current.s_addr) <= ntohl(data->end.s_addr)) { 
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &current, ip_str, INET_ADDRSTRLEN);
        
        if (is_ip_active(ip_str)) { 




            //MUTEX
            if ((r = pthread_mutex_lock(&my_mutex)) != 0)
            {
                fprintf(stderr, "Mutex lock error: %s\n", strerror(r));
                pthread_exit(NULL);
            }
            
            fprintf(outf, "\nIP %s este activ -> scanare porturi:\n", ip_str);

            get_mac_address(ip_str);

            portData port_data = {ip_str, data->start_port, data->end_port};
            pthread_create(&port_thread, NULL, scan_ports, (void*)&port_data);
            pthread_join(port_thread, NULL);


            if ((r = pthread_mutex_unlock(&my_mutex)) != 0)
            {
                fprintf(stderr, "Mutex unlock error: %s\n", strerror(r));
                pthread_exit(NULL);
            }





        } else { 
            printf("IP %s is not active\n", ip_str);
        }
        
        current.s_addr = htonl(ntohl(current.s_addr) + 1);
    }
    free(data);
    return NULL;
}



int main(int argc, char **argv) { 

    struct in_addr start, end;

    // ./p   192.168.1.1   192.168.1.80       1           100             10
    //          startIP         endIP     startPort     endPort        nrThreads
    //   0         1              2           3             4              5
    if (argc < 1 || argc > 6) { 
        perror("nr arg gresit (min 1 max 6)\n");
        exit(1);
    }

    
    if (argc > 1) { 
        if (inet_pton(AF_INET, argv[1], &start) <= 0) { 
            perror("start ip gresit\n");
            exit(2);
        }
    } else { 
        inet_pton(AF_INET, default_ip_start, &start);
    }

    if (argc > 2) { 
        if (inet_pton(AF_INET, argv[2], &end) <= 0) { 
            perror("final ip gresit\n");
            exit(3);
        }
    } else { 
        inet_pton(AF_INET, default_ip_end, &end);
    }

    int start_port;
    if (argc > 3) 
        start_port = atoi(argv[3]);
    else 
        start_port = default_port_start;
    

    int end_port;
    if (argc > 4) 
        end_port = atoi(argv[4]);
    else 
        end_port = default_port_end;
    

    int thread_count;
    if (argc > 5) 
        thread_count = atoi(argv[5]);
    else
        thread_count = default_no_threads;
    
    if (thread_count <= 0) { 
        perror("err - trebuie cel putin 1 thread");
        exit(4);
    }

    if((outf = fopen("output.txt", "w"))==NULL) {
        perror("err fopen\n");
        exit(155);
    }
    

    uint32_t startIP = ntohl(start.s_addr);
    uint32_t endIP = ntohl(end.s_addr);
    uint32_t total_ips;

    if (startIP <= endIP)
        total_ips = endIP - startIP + 1;
    else 
        total_ips = 0;
    
    if (total_ips == 0) { 
        perror("interval ip gresit");
        exit(5);
    }

    uint32_t ips_per_thread = total_ips / thread_count;

    if (ips_per_thread == 0) { 
        perror("interval ip prea mic");
        exit(6);
    }

    pthread_t th[thread_count];
    uint32_t current_start = startIP;
    
    for (int i = 0; i < thread_count; i++) { 
        IPData *data = malloc(sizeof(IPData));
        if (data == NULL) { 
            perror("err malloc");
            exit(7);
        }

        data->start.s_addr = htonl(current_start);

        if (i == thread_count - 1) { 
            data->end.s_addr = htonl(endIP);
        } else { 
            data->end.s_addr = htonl(current_start + ips_per_thread - 1);
        }


        data->start_port = start_port;
        data->end_port = end_port;


        current_start = ntohl(data->end.s_addr) + 1;

        if (pthread_create(&th[i], NULL, scan_ip_range, data) != 0) { 
            perror("err pthread_create");
            free(data);
            exit(8);
        }
        
    }

    for (int i = 0; i < thread_count; i++) { 
        pthread_join(th[i], NULL);
    }



    if(fclose(outf)!=0) {
        perror("err fclose\n");
        exit(152);
    }
    return 0;
}
