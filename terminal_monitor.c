/*
 * @Description: 
 * @Version: 1.0
 * @Autor: Ming
 * @Date: 2021-01-17 22:40:46
 * @LastEditors: Ming
 * @LastEditTime: 2021-01-17 23:50:20
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/sysinfo.h>
//多线程处理库
#include <pthread.h>
//网络监控部分
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "terminal_monitor.h"

/*文件/proc/net/dev*/
FILE *net_dev_file = NULL;
/*本机网络接口信息结构体*/
NET_INTERFACE *p_interface = NULL;

HOST_STATE host_core;

void *thread_net(void *arg)
{
    printf("%s\n", __FUNCTION__);
    while (1)
    {
        get_network_speed(p_interface);
        show_netinterfaces(p_interface, 1);
    }
}

void *thread_core(void *arg)
{
    printf("%s\n", __FUNCTION__);
    while (1)
    {
        open_sysinfo();

        get_host_runtime(&host_core.hour, &host_core.minute);
        printf("[run]:%6d hours\t%2d minutes\t", host_core.hour, host_core.minute);
        
        get_mem_usage(&host_core.mem_used);
        printf("[mem_used]:%6.2f%%\t", host_core.mem_used);

        get_cpu_usage(&(host_core.cpu_used));
        printf("[cpu_used]:%6.2f%%\n", host_core.cpu_used);
        sleep(1);
    }
}

void get_cpuoccupy(CPU_OCCUPY *cpu)
{

    char buff[BUFFSIZE];
    FILE *fd;

    /* /proc/stat 包含了系统启动以来的许多关于kernel和系统的统计信息，
    其中包括CPU运行情况、中断统计、启动时间、上下文切换次数、
    运行中的进程等等信息 */
    fd = fopen("/proc/stat", "r");
    if (fd == NULL)
    {
        perror("open /proc/stat failed\n");
        exit(0);
    }
    fgets(buff, sizeof(buff), fd);

    sscanf(buff, "%s %u %u %u %u", cpu->name, &cpu->user, &cpu->nice, &cpu->system, &cpu->idle);
    fclose(fd);
}

double cal_occupy(CPU_OCCUPY *c1, CPU_OCCUPY *c2)
{
    double t1, t2;
    double id, sd;
    double cpu_used;

    t1 = (double)(c1->user + c1->nice + c1->system + c1->idle);
    t2 = (double)(c2->user + c2->nice + c2->system + c2->idle);

    id = (double)(c2->user - c1->user);
    sd = (double)(c2->system - c1->system);

    cpu_used = (100 * (id + sd) / (t2 - t1));
    return cpu_used;
}

void open_sysinfo()
{
    int ret = 0;
    ret = sysinfo(&info);
    if (ret != 0)
    {
        perror("get sys_info failed\n");
        exit(0);
    }
}

void get_mem_usage(double *mem_used)
{
    *mem_used = (info.totalram - info.freeram) * 100.0 / info.totalram;
#if DEBUG
    printf("total memory: %lu\n", info.totalram);
    printf("free memory: %lu\n", info.freeram);
    printf("memory usage: %.2f%\n", mem_used);
#endif
}

void get_cpu_usage(double *cpu_used)
{
    CPU_OCCUPY cpu_0, cpu_1;

    get_cpuoccupy(&cpu_0);
    /*   while (1)  {
    //实现循环监控CPU状态，间隔时间1s */
    sleep(1);
    get_cpuoccupy(&cpu_1);

    *cpu_used = cal_occupy(&cpu_0, &cpu_1);
#if DEBUG
    printf("cpu: %.2f%\n", *cpu_used);
#endif
    /*   cpu_0 = cpu_1;//循环监控打开，结构体拷贝赋值语句
  } */
}

void get_host_runtime(int *hours, int *minutes)
{

    *hours = info.uptime / 3600;
    *minutes = (info.uptime % 3600) / 60;
#if DEBUG
    printf("run time: %ld_hours %d_minutes\n", *hours, *minutes);
#endif
}

void open_netconf(FILE **fd)
{
    *fd = fopen("/proc/net/dev", "r");
    if (*fd == NULL)
    {
        perror("open file /proc/net/dev failed!\n");
        exit(0);
    }
}

void show_netinterfaces(NET_INTERFACE *p_net, int n)
{
    NET_INTERFACE *temp;
    temp = p_net;

    while (temp != NULL)
    {
        if (!n)
        {
            printf("Interface:%8s\t IP:%16s\t MAC:%12s\n", temp->name, temp->ip, temp->mac);
        }
        else
        {
            printf("Interface:%8s\t", temp->name);
            if ((temp->speed_level >> 1) & 0x1)
            {
                printf("download:%10.2lfMB/s\t", temp->d_speed);
            }
            else
            {
                printf("download:%10.2lfKB/s\t\t", temp->d_speed);
            }

            if (temp->speed_level & 0x1)
            {
                printf("upload:%10.2lfMB/s\n", temp->u_speed);
            }
            else
            {
                printf("upload:%10.2lfKB/s\n", temp->u_speed);
            }
        }

        temp = temp->next;
    }
}

int get_interface_info(NET_INTERFACE **net, int *n)
{
    int fd;
    int num = 0;
    struct ifreq buf[16];
    struct ifconf ifc;

    NET_INTERFACE *p_temp = NULL;
    (*net)->next = NULL;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        close(fd);
        printf("socket open failed\n");
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    {
        //get interface nums
        num = ifc.ifc_len / sizeof(struct ifreq);
        *n = num;

        //find all interfaces;
        while (num-- > 0)
        {
            // exclude lo interface
            /* if (!strcmp("lo", buf[num].ifr_name))
        continue; */

            //get interface name
            strcpy((*net)->name, buf[num].ifr_name);
#if DEBUG
            printf("name:%8s\t", (*net)->name);
#endif
            //get the ipaddress of the interface
            if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[num])))
            {

                memset((*net)->ip, 0, 16);
                strcpy((*net)->ip, inet_ntoa(((struct sockaddr_in *)(&buf[num].ifr_addr))->sin_addr));
#if DEBUG
                printf("IP:%16s\t", (*net)->ip);
#endif
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[num])))
            {

                memset((*net)->mac, 0, 13);

                snprintf((*net)->mac, 13, "%02x%02x%02x%02x%02x%02x",
                         (unsigned char)buf[num].ifr_hwaddr.sa_data[0],
                         (unsigned char)buf[num].ifr_hwaddr.sa_data[1],
                         (unsigned char)buf[num].ifr_hwaddr.sa_data[2],
                         (unsigned char)buf[num].ifr_hwaddr.sa_data[3],
                         (unsigned char)buf[num].ifr_hwaddr.sa_data[4],
                         (unsigned char)buf[num].ifr_hwaddr.sa_data[5]);
#if DEBUG
                printf("mac:%12s\n", (*net)->mac);
#endif
            }
            if (num >= 1)
            {
                p_temp = (NET_INTERFACE *)malloc(sizeof(NET_INTERFACE));
                memset(p_temp, 0, sizeof(NET_INTERFACE));
                p_temp->next = *net;
                *net = p_temp;
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

void get_rtx_bytes(char *name, RTX_BYTES *rtx)
{
    char *line = NULL;

    size_t bytes_read;
    size_t read;

    char str1[32];
    char str2[32];

    int i = 0;
    open_netconf(&net_dev_file);
    //获取时间
    gettimeofday(&rtx->rtx_time, NULL);
    //从第三行开始读取网络接口数据
    while ((read = getline(&line, &bytes_read, net_dev_file)) != -1)
    {
        if ((++i) <= 2)
            continue;
        if (strstr(line, name) != NULL)
        {
            memset(str1, 0x0, 32);
            memset(str2, 0x0, 32);

            sscanf(line, "%*s%s%*s%*s%*s%*s%*s%*s%*s%s", str1, str2);

            rtx->tx_bytes = atol(str2);
            rtx->rx_bytes = atol(str1);
#if DEBUG
            printf("name:%8s\t tx:%10ld\trx:%10ld\n", name, rtx->tx_bytes, rtx->rx_bytes);
#endif
        }
    }
    fclose(net_dev_file);
}

void rtx_bytes_copy(RTX_BYTES *dest, RTX_BYTES *src)
{
    dest->rx_bytes = src->rx_bytes;
    dest->tx_bytes = src->tx_bytes;
    dest->rtx_time = src->rtx_time;
    src->rx_bytes = 0;
    src->tx_bytes = 0;
}

void get_network_speed(NET_INTERFACE *p_net)
{

    RTX_BYTES rtx0, rtx1;

    NET_INTERFACE *temp;
    temp = p_net;
    while (temp != NULL)
    {
        get_rtx_bytes(temp->name, &rtx0);
        /*  while (1)
    { */
        sleep(WAIT_SECOND);
        get_rtx_bytes(temp->name, &rtx1);
		temp->speed_level &= 0x00;
        cal_netinterface_speed(&temp->u_speed, &temp->d_speed, &temp->speed_level,
                               &rtx0, &rtx1);

        /*      rtx_bytes_copy(&rtx0, &rtx1);
    } */
        temp = temp->next;
    }
}

void cal_netinterface_speed(double *u_speed, double *d_speed, unsigned char *level, RTX_BYTES *rtx0, RTX_BYTES *rtx1)
{
    long int time_lapse;

    time_lapse = (rtx1->rtx_time.tv_sec * 1000 + rtx1->rtx_time.tv_usec / 1000) - (rtx0->rtx_time.tv_sec * 1000 + rtx0->rtx_time.tv_usec / 1000);

    *d_speed = (rtx1->rx_bytes - rtx0->rx_bytes) * 1.0 / (1024 * time_lapse * 1.0 / 1000);
    *u_speed = (rtx1->tx_bytes - rtx0->tx_bytes) * 1.0 / (1024 * time_lapse * 1.0 / 1000);

    //speed_level 置0
    //    *level &= 0x00;

    if (*d_speed >= MB * 1.0)
    {
        *d_speed = *d_speed / 1024;
        *level |= 0x1;
#if DEBUG
        printf("download:%10.2lfMB/s\t\t", *d_speed);
#endif
    }
    else
    {
        //定义速度级别
        *level &= 0xFE;
#if DEBUG
        printf("download:%10.2lfKB/s\t\t", *d_speed);
#endif
    }

    if (*u_speed >= MB * 1.0)
    {
        *u_speed = *u_speed / 1024;
        *level |= 0x1 << 1;
#if DEBUG
        printf("upload:%10.2lfMB/s\n", *u_speed);
#endif
    }
    else
    {
        //定义速度级别
        *level &= 0xFD;
#if DEBUG
        printf("upload:%10.2lfKB/s\n", *u_speed);
#endif
    }
}

int main()
{

    int nums = 0;
    //网卡结构体指针初始化
    p_interface = (NET_INTERFACE *)malloc(sizeof(NET_INTERFACE));
    //获取本机网卡数量
    get_interface_info(&p_interface, &nums);
    printf("net_interface nums: %d\n", nums);

    show_netinterfaces(p_interface, 0);
    //创建两个线程
    pthread_t thread_net_id, thread_core_id;
    //线程1：网络信息监控线程
    pthread_create(&thread_net_id, NULL, (void *)thread_net, NULL);
    //线程2：CPU、内存信息监控
    pthread_create(&thread_core_id, NULL, (void *)thread_core, NULL);

    while (1)
    {
        sleep(1);
    }
    return 0;
}
