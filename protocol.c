
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>


#define FALSE 0
#define TRUE 1

#define TRASMITTER 0
#define RECEIVER 1

#define FLAG 0x7E
#define SUPERVISION_FRAME_SIZE 5
//COMANDOS SET E DISC
#define SET 0x03
#define DISC 0x11


//RESPOSTAS UA RR REJ
#define UA 0x07
#define RR0 2
#define RR1 2
#define REJ0 3
#define REJ1 3

#define NUMBER_ATTEMPTS 3
#define ALARM_WAIT_TIME 3

struct linkLayer {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int baudRate; /*Velocidade de transmissão*/
    unsigned int sequenceNumber;   /*Número de sequência da trama: 0, 1*/
    unsigned int timeout; /*Valor do temporizador: 1 s*/
    unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
    char frame[MAX_SIZE]; /*Trama*/
}

struct linkLayer l1;


int flag=1, try=1;

void alarmHandler()                   // atende alarme
{
    printf("Sending again. Try: # %d\n", try);
    flag=1;
    try++;
}

//Creates Supervision and Unnumbered Frames
int createSuperVisionFrame (int option, char controlField){
    l1.frame[0] = FLAG;
    if(option == TRASMITTER){
        if (controlField == (SET || DISC)) l1.frame[1]=0x03; 
        else if(controlField == (UA || RR0 || RR1 || REJ0 || REJ1 )) l1.frame[1]=0x01;
    }
    else if(option == RECEIVER){
        if (controlField == (SET || DISC)) l1.frame[1]=0x01; 
        else if(controlField == (UA || RR0 || RR1 || REJ0 || REJ1 )) l1.frame[1]=0x03;
    }
    l1.frame[2]=controlField;
    l1.frame[3]=l1.frame[1]^l1.frame[2];
    l1.frame[4]=FLAG;
}


int sendFrame(int fd, int frameSize,char responseControlField){
    char responseBuffer[5];

    int FLAG_RCV=FALSE;
    int A_RCV=FALSE;
    int C_RCV=FALSE;
    int BCC_OK=FALSE;
    
    int STOP = FALSE;
    while (!STOP && try<=NUMBER_ATTEMPTS){
        if(flag){
            for (int i=0;i <= frameSize;i++){
                write(fd,&l1.frame[i],1);
            }
            alarm(ALARM_WAIT_TIME);
            flag=0;
        }
        read(fd,responseBuffer,1);
        switch (responseBuffer[0]) {
            case FLAG:
                FLAG_RCV=TRUE;
                if(A_RCV&&C_RCV&&BCC_OK&&FLAG_RCV){
                    STOP=TRUE;
                }
                printf("%x\n",buf[0]);
                break;
            case (0x03): 
                if(FLAG_RCV&&!A_RCV)
                    A_RCV=TRUE;
                printf("%x\n",buf[0]);
                break;
            case responseControlField:
                if(FLAG_RCV&&A_RCV)
                    C_RCV=TRUE;
                break;
            case (0x03^responseControlField):
                if(FLAG_RCV&&A_RCV&&C_RCV){
                    BCC_OK=TRUE;
                    printf("%x\n",buf[0]);
                }
                break;
            default:
                break;
        }
    }
    if(try==NUMBER_ATTEMPTS) return -1;
    else return 0;
}


int llopen(int port, int option){
    int fd;
    struct termios oldtio,newtio;
    fd = open(port, O_RDWR | O_NOCTTY );
    if (fd <0) {
        perror(port);
        exit(-1); 
    }
    if(option == TRASMITTER){
        (void) signal(SIGALRM, alarmHandler); //Alarm setup

        createSuperVisionFrame(option,SET); //Creates the frame to send

        if (sendFrame(fd, SUPERVISION_FRAME_SIZE, UA)<0){ //Sends the frame to the receiver
            printf("No response received. Gave up after %d tries",NUMBER_ATTEMPTS);
            exit(1);
        }

    }
    else if(option == RECEIVER){
        createSuperVisionFrame(option,UA);

        if(sendFrame(fd, SUPERVISION_FRAME_SIZE,))
    }


    return 0;
}
