#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#define MAX 1024
#define PATH_SIZE 128
int main(void)
{
	DIR *dir;
	struct dirent *entry;
	FILE *fp;
	char path[PATH_SIZE];
	char buf[MAX];
	char target[20] = "soffice.bin"; 
	int flag;	

	while(1){
		flag = 0;//初始假设进程不存在		

		if((dir = opendir( "/proc" )) == NULL ) { // 打开/proc目录 
        	        perror("fail to open dir");
                	return -1; 
        	}

		while((entry = readdir( dir ) ) != NULL){
			if(entry->d_name[0] == '.') // 跳过当前目录，proc目录没有父目录 
				continue;
			// 跳过系统信息目录，所有进程的目录全都是数字，而系统信息目录全都不是数字 
			if( (entry->d_name[0] <='0' ) || (entry->d_name[0] >= '9'))
				continue;
			// 使用sprintf完成拼接路径，其中两个%s会由entry->d_name表示的进程ID替代 
			sprintf(path, "/proc/%s/task/%s/status", entry->d_name,entry->d_name);
			printf("%s\n",path);
			fp = fopen(path, "r"); // 打开文件
			if(fp == NULL){
//				printf("路径%s\n",path);				
//				printf("进程号%s\n",entry->d_name);
				perror("fail to open");
				exit(1);
			}

			fgets(buf, MAX, fp);//取出文件的第一行存入buf中
			fclose(fp); // 关闭status文件 
			 printf("%s\n",buf);
//			printf("%s %s \n",buf,target);

			if ( strstr(buf,target) != NULL )
                	{
				 printf("路径%s\n",path);                                
                                 printf("进程号%s\n",entry->d_name);
				 printf("缓冲区%s\n",buf);
                         	 flag = 1;//进程存在
			 	 break;                                       
                	} 

		}

		closedir( dir ); // 关闭目录 
	
		if(flag == 0){//进程不存在，重启进程
                	system("/usr/lib/libreoffice/program/soffice.bin");
         	}
		
		sleep(1);
	}
	return 0;
}




/*
现象：
	启动psdaemon之前，若libreoffice已经在运行，则psdaemon运行过程中关闭libreoffice会出现文件不存在无法打开文件的错误，程序异常终止。
    　　启动psdaemon之前，若libreoffice没有在运行，则psdaemon运行过程中关闭libreoffice后程序可以实现重启libreoffice，符合预期。
 
调试及结论：
	step1:在打开文件之前加入用于输出文件路径的语句，在kill掉libreoffice进程之前先查看进程号。运行结果表示找不到的文件正好就是刚被kill掉的libreoffice进程的status文件。也就是进程被kill掉之后相应的文件没能及时删除就进入了下一轮while循环。一轮while循环实在太迅速了。
        step2:输出文件名和进程status文件中包含的进程name信息，发现如果提前打开了libreoffice则会出现期待的有关soffice.bin的路径、进程号、name等输出，而未提前打开libreoffice的话每次都没有相应的输出。后者的是，发现libreoffice没在运行之后，启动libreoffice，之后程序一直处于libreoffice的状态下了，只要libreoffice在运行，下一轮while循环就不会运行，只有kill掉libreoffice之后才会又进入下一轮的while循环。

这么说来的话，在同样没有sleep（或usleep)帮忙挂起进程的情况下，两种情况在进入下一轮while循环之前经历的时间分别为：
１、之前libreoffice已经在运行：
	上一轮while循环判断进程还是在的，下一轮再看的时候status文件被删除了，可是相应的上几级文件夹还没来得及删除干净，导致进入文件夹发现status文件没有存在。所以也就只隔着上一层循环判断flag是否为０的语句和本次大循环遍历到被kill掉的office进程文件夹打开status文件之前的几句。

２、libreoffice是后期被打开的：
	这样的话，libreoffice进程其实是作为psdaemon进程的子进程的，是不是子进程被kill掉之后，父进程还做了一些处理耗费了时间，再加上到达下一次while大循环的遍历到被kill掉的office进程文件夹打开status文件之前的时间，这样足够系统完全清理libreoffice进程在proc文件夹中的所有相关文件了，所以在下一次while循环的时候就没有出现进入文件夹却发现status文件不存在的情况。

这样的结论的前提是：
１、操作系统删除文件是由内而外的，从里往外一层层的删除
２、父进程在子进程被kill掉之后还要做一些处理，因此耗费一些时间。

误打误撞的收获:
1、用sleep或者usleep挂起进程
2、不需要额外的终端来kill掉psdaemon进程，两次“Ｃｔｒｌ＋Ｃ”就可以了
*/
