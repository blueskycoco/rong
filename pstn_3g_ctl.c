#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <dirent.h>
static int run=1;

static void stop(int signum)
{
	run=0;
	printf("to stop exe\r\n");
}

int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	if  ( tcgetattr( fd,&oldtio)  !=  0) { 
		perror("SetupSerial 1");
		return -1;
	}
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag  |=  CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 

	switch( nBits )
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
	}

	switch( nEvent )
	{
		case 'O':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E': 
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;
		case 'N':  
			newtio.c_cflag &= ~PARENB;
			break;
	}

	switch( nSpeed )
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		default:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
	}
	if( nStop == 1 )
		newtio.c_cflag &=  ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |=  CSTOPB;
	newtio.c_cc[VTIME]  = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd,TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	printf("set done!\n");
	return 0;
}

int open_com_port(int comport)
{
	int fd;
	long  vdisable;
	if (comport==0)
	{	
		fd = open( "/dev/ttyS0", O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd){
			perror("Can't Open Serial Port0");
			return(-1);
		}
		else 
			printf("open tts/0 .....\n");
	}
	else if(comport==1)
	{	
		fd = open( "/dev/ttyS1", O_RDWR/*|O_NOCTTY|O_NDELAY*/);
		if (-1 == fd){
			perror("Can't Open Serial Port2");
			return(-1);
		}
		else 
			printf("open tts/1 .....\n");
	}
	else if (comport==2)
	{
		fd = open( "/dev/ttyS2", O_RDWR/*|O_NOCTTY|O_NDELAY*/);
		if (-1 == fd){
			perror("Can't Open Serial Port3");
			return(-1);
		}
		else 
			printf("open tts/2 .....\n");
	}
	if(fcntl(fd, F_SETFL, FNDELAY)<0)
		printf("fcntl failed!\n");
	else
		printf("fcntl=%d\n",fcntl(fd, F_SETFL,FNDELAY));
	if(isatty(STDIN_FILENO)==0)

		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");
	printf("fd-open=%d\n",fd);
	return fd;
}
int phone_process(int fd,int type,char *phone_number)
{
	int nread,nwrite,i,j;
	char buff0[3]="ATD";
	char buff1[3]="ATA";
	char buff2[7]="AT+CHUP";
	char buff3[9]="AT+CLIP=1";
	char end='\r';
	char end_sent=';';
	if(type==0)//call out
	{
		printf("call Phone %s through serial fd %d\r\n",phone_number,fd);	
		write(fd,buff0,3);
		write(fd,phone_number,strlen(phone_number));
		write(fd,&end_sent,1);
		write(fd,&end,1);
	}
	else if(type==1)//open call in display
	{
		printf("open incoming phone number display on fd %d\r\n",fd);	
		write(fd,buff3,9);
		write(fd,&end,1);
	}
	else if(type==2)//reject call in
	{
		printf("reject incoming call on fd %d\r\n",fd);	
		write(fd,buff2,7);
		write(fd,&end,1);
	}
	else if(type==3)//accept call in
	{
		printf("accept incoming call on fd %d\r\n",fd);	
		write(fd,buff1,3);
		write(fd,&end,1);
	}
	return 0;
}
int wait_phone_call(int fd_3g1,int fd_pstn , int fd_3g2,char **out)
{
	char buf[256],*buf2;
	char ch,my_count=0;
	int i=0,z=0,result=0;
	memset(buf,'\0',256);
	//check Calling in from 3G1
	while(read(fd_3g1,&ch,1)==1)
	{
		buf[i++]=ch;
		if(ch=='"')
			my_count++;
	}
	if(my_count==2||i!=0)
	{
		buf[i]='\0';
		printf("3G1 in %s\r\n",buf);
		if(/*strncmp(buf,"\r\n+CLIP:",8)==0*/my_count==2)
		{
			int j=0;
			*out=(char *)malloc(20*sizeof(char));
			buf2=*out;  
			memset(buf2,'\0',20);
			i=0;
			while(buf[j]!='\"' && j<i)
			{
				j++;
			}
			j++;
			z=1;
			while(buf[z]!='\"' && j<i)
				buf2[z++]=buf[j++];
			if(j==i)
				return -1;
			buf2[z]='\0';
			buf2[0]='1';
			printf("Calling in 3G1 %s\r\n",*out);
			return 1;
		}
		else if(strncmp(buf,"\r\n^CEND",7)==0)
		{/*hang up info*/
			//printf("no Ring in from 3g1\r\n");
				printf("3G1 in CEND\r\n");
				*out=(char *)malloc(sizeof(char));
				*out[0]='3';
				return 3;
		}
	}
	//check Calling in from PSTN
	i=0;
	/*while(read(fd_pstn,&ch,1)==1)
	{
		printf("pstn:%c\r\n",ch);
		buf[i++]=ch;
	}*/
	char *ptr=buf;
	i=read(fd_pstn,ptr,100);
	if(i!=0)
	{
		*(ptr+i)='\0';
		*out=(char *)malloc(20*sizeof(char));
		buf2=*out;  
		memset(buf2,'\0',20);
		memcpy((void *)(*out+1),ptr,i);
		buf2[i+1]='\0';
		buf2[0]='2';
		printf("Calling in PSTN %s\r\n",*out);
		return 2;
	}
	//check 3g2 hang up
	i=0;
	while(read(fd_3g2,&ch,1)==1)
	{
		buf[i++]=ch;
	}
	if(i!=0)
	{
		buf[i]='\0';
		printf("3G2 in %s\r\n",buf);
		if(strncmp(buf,"\r\n^CEND",7)==0)
		{
			printf("3G2 in CEND\r\n");
			*out=(char *)malloc(sizeof(char));
			*out[0]='4';
			return 4;
		}
	}
	//return 4;
	return -1;
}
void print_system_status(int status)
{
	printf("status = %d\n",status);
	if(WIFEXITED(status))
	{
		printf("normal termination,exit status = %d\n",WEXITSTATUS(status));
	}
	else if(WIFSIGNALED(status))
	{
		printf("abnormal termination,signal number =%d%s\n",
		WTERMSIG(status),
		WCOREDUMP(status)?"core file generated" : "");
	}
}
int init_mixer()
{
	char command[256];
	int result;
	sprintf(command,"%s","/usr/local/bin/bin/amixer -c 0 sset \'Analog Right Sub Mic\' cap");
	printf("open_record %s\r\n",command);
	print_system_status(system(command));
	return result;
}
int main(int argc,char *argv[])
{
	pid_t fpid;
	int fd_3g1=0;
	int fd_3g2=0;
	int fd_pstn=0;
	int father_pid=getpid();
	int child_pid;
	init_mixer();
	if((fd_3g2=open_com_port(2))<0)
	{
		perror("pstn_3g_ctl father process open_port error");
		return -1;
	}
	if(set_opt(fd_3g2,115200,8,'N',1)<0)
	{
		perror("pstn_3g_ctl father process set_opt error");
		return -1;
	}
	
	if((fd_3g1=open_com_port(1))<0)
	{
		perror("pstn_3g_ctl father process open_port error");
		return -1;
	}
	if(set_opt(fd_3g1,115200,8,'N',1)<0)
	{
		perror("pstn_3g_ctl father process set_opt error");
		return -1;
	}
	fd_pstn = open( "/dev/cmx865a", O_RDWR|O_NOCTTY|O_NDELAY);
	if (-1 == fd_pstn){
		perror("Can't Open PSTN");
	}
	else 
		printf("open PSTN .....\n");
	//open 3g phone num display
	phone_process(fd_3g1,1,NULL);			
	phone_process(fd_3g1,2,NULL);
	phone_process(fd_3g2,2,NULL);
	char init_pstn=0;
	write(fd_pstn, &init_pstn, sizeof(char)); 

	fpid=fork();
	if(fpid<0)
		printf("pstn_3g_ctl fork failed\r\n");
	else if(fpid==0)
	{
		/*in child process,accept or reject 3g1,pstn call, get phone num from sipvg ,dial 3g2 out,or hang up*/	
		signal(SIGINT,stop);
		child_pid=getpid();
		const char *fifo_name_r = "/tmp/from_sipvg";	
		if(access(fifo_name_r, F_OK) == -1)  
    	{          
	        int res = mkfifo(fifo_name_r, 0777);  
	        if(res != 0)  
	        {  
	            fprintf(stderr, "pstn_3g_ctl child process %d Could not create fifo %s\n",father_pid, fifo_name_r);  
	            exit(-1);  
	        }  
    	}  
		int pipe_fd_r = open(fifo_name_r, O_RDONLY/*|O_NONBLOCK*/); 
		printf("pstn_3g_ctl child process %d open fifo over\r\n",child_pid);
		char command[PIPE_BUF+1],read_bytes,hang_up_pstn,*ptr;
		while(run)
		{
			 memset(command,'\0',sizeof(command));
			 ptr=command;
			 read_bytes = read(pipe_fd_r, ptr, PIPE_BUF);
			 
			 if(read_bytes>0)
			 {
			 printf("pstn read_bytes %d,%s\r\n",read_bytes,ptr);
			 switch(*ptr)
			 {
				case 0://hang up 3g1 and pstn and 3g2 call
				{
					printf("pstn_3g_ctl child process %d hang up 3g1 and pstn and 3g2 call\r\n",child_pid);
					phone_process(fd_3g1,2,NULL);
					phone_process(fd_3g2,2,NULL);
					hang_up_pstn=0;
					write(fd_pstn, &hang_up_pstn, sizeof(char));  
				}
				break;
				case 1://accept 3g1 and dial 3g2 out from command[1]
				{
					printf("pstn_3g_ctl child process %d accept 3g1 and dial 3g2 out %s\r\n",child_pid,(char *)(ptr+1));
					phone_process(fd_3g1,3,NULL);
					phone_process(fd_3g2,0,(char *)(ptr+1));
				}
				break;
				case 2://accept pstn and dial 3g2 out from command[1]
				{
					printf("pstn_3g_ctl child process %d accept pstn and dial 3g2 out %s\r\n",child_pid,(char *)(ptr+1));
					hang_up_pstn=1;
					write(fd_pstn, &hang_up_pstn, sizeof(char));  
					phone_process(fd_3g2,0,(char *)(ptr+1));
				}
				break;
				case 3://accept voip and dial 3g2 out from command[1]
				{
					printf("pstn_3g_ctl child process %d accept voip and dial 3g2 out %s\r\n",child_pid,(char *)(ptr+1));	
					phone_process(fd_3g2,0,(char *)(ptr+1));
				}
				break;
			 }
			}
		}
		printf("pstn_3g_ctl child process %d exit\r\n",child_pid);
		close(pipe_fd_r);
		return 0;
	}
	else
	{
		/*in father process,mon 3g1,pstn coming in call,and transfer to sipvg process*/	
		signal(SIGINT,stop);
		const char *fifo_name_w = "/tmp/to_sipvg";
		if(access(fifo_name_w, F_OK) == -1)  
    	{          
	        int res = mkfifo(fifo_name_w, 0777);  
	        if(res != 0)  
	        {  
	            fprintf(stderr, "pstn_3g_ctl father process %d Could not create fifo %s\n",father_pid, fifo_name_w);  
	            exit(-1);  
	        }  
    	}  
		int pipe_fd_w = open(fifo_name_w, O_WRONLY/*|O_NONBLOCK*/); 
		printf("pstn_3g_ctl father process %d open fifo over\r\n",father_pid);
		char phone_num[15],*ptr;/*first byte is source(3g1 0 call in or pstn 1 call in . 3g1 2 hang up call  or 3g2 3 hang up call),last is phone_num*/
		int write_bytes,len;
		while(run)
		{		
			memset(phone_num,'\0',strlen(phone_num));
			ptr=phone_num;
			if(wait_phone_call(fd_3g1,fd_pstn,fd_3g2,&ptr)!=-1)
			{
				if(*ptr=='1'||*ptr=='2')
					printf("pstn_3g_ctl father process %d ,have %s in coming call %s\r\n",father_pid,(*ptr=='1')?"3G1":"PSTN",(char *)(ptr+1));
				else
					printf("pstn_3g_ctl father process %d ,have %s hang up call %s\r\n",father_pid,(*ptr=='3')?"3G1":"3G2",(char *)(ptr+1));
				len=strlen(ptr);
				printf("Send command to sipvg %s,len %d\r\n",ptr,len);
				write_bytes = write(pipe_fd_w, ptr, len);  
	            if(write_bytes == -1)  
	            {  
	                //fprintf(stderr, "pstn_3g_ctl father process %d Write error on pipe\n",father_pid);  
	                //return -1;
	            }
			}
		}
		char exit='5';
		write(pipe_fd_w, &exit, sizeof(char));  
		printf("Send command to sipvg %c,len %d\r\n",exit,sizeof(char));
        if(write_bytes == -1)  
        {  
            fprintf(stderr, "pstn_3g_ctl father process %d Write error on pipe\n",father_pid);  
            return -1;
        }
		close(pipe_fd_w);
		close(fd_3g1);
		close(fd_3g2);
		close(fd_pstn);
	}
	int pr;
	do
	{
		pr=waitpid(fpid,NULL,WNOHANG);
		sleep(1);
	}while(pr!=fpid);
	printf("get child exit\r\n");
	printf("pstn_3g_ctl father process %d exit\r\n",father_pid);
#if 0
	int i=0,k=0,m=0,next_write=0;
	char ch;
	FILE *fp=NULL;
	char *in=NULL;
	printf("\r\nPhone System\r\n");
	open_record();
	if((g_fd_1=open_port(1))<0){
		perror("open_port error 1");
		return -1;
	}
	if(set_opt(g_fd_1,115200,8,'N',1)<0){
		perror("set_opt error 1");
		return -1;
	}
	if((g_fd_2=open_port(2))<0){
		perror("open_port error 2");
		return -1;
	}
	if(set_opt(g_fd_2,115200,8,'N',1)<0){
		perror("set_opt error 2");
		return -1;
	}
	for(i=0;i<phone_map_len;i++)
	{
		memset(phone_in[i],'\0',sizeof(char)*20);
		memset(phone_out[i],'\0',sizeof(char)*20);
	}
	fp=fopen("./phone.txt","r");
	if(fp<0)
	{
		perror("open phone.txt failed\r\n");
		return -1;
	}
	while(fread(&ch,sizeof(char),1,fp)==1)
	{
		if(ch==',')
		{/*next is route out phone number*/
			next_write=1;
			m=0;
		}
		else if(ch=='\n')
		{/*next is call in phone number*/
			next_write=0;
			k++;
			m=0;
		}
		else
		{/*store in phone_map*/
			if(next_write==1)
			{
				phone_out[k][m++]=ch;
			}
			else
			{
				phone_in[k][m++]=ch;
			}
		}
	}
	fclose(fp);
	k=k-1;
	for(i=0;i<k;i++)
	{
		printf("Phone[%d] in: %s <==> Phone[%d] out : %s\r\n",i,phone_in[i],i,phone_out[i]); 
	}
	phone_process(g_fd_1,1,NULL);
	//phone_process(g_fd_2,1,NULL);
	/*wait for phone call in */
	while(1)
	{
		int source=wait_phone_call(&in);
		if(source!=0)
		{
			for(i=0;i<k;i++)
			{
				printf("in %s <> phone_in list %s\r\n",in,phone_in[i]);
				if(strncmp(in,phone_in[i],strlen(in))==0)
				{
					printf("accept %s \r\nrout out %s\r\n",phone_in[i],phone_out[i]);
					phone_process(g_fd_2,3,NULL);//accept call
					phone_process(g_fd_1,0,phone_out[i]);//route to out call
					/*record voice from g_fd_2 , play to g_fd_1 */
					if(source!=4)
						voice_route(source,2);
					else
						voip_route();
					break;
				}
				if(i==k-1)
				{
					printf("incoming call not in white list ,reject it\r\n");
					phone_process(g_fd_2,2,NULL);
				}
			}
			free(in);
			in=NULL;
		}
		else
		{
			sleep(1);
//			printf("No call in ,waiting...\r\n");
		}
	}
	#endif
	//wait_pid(child_pid);
	return 0;
}

