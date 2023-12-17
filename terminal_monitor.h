/*
 * @Description: terminal_monitor.h
 * @Version: 1.0
 * @Autor: Ming
 * @Date: 2021-01-17 22:40:59
 * @LastEditors: Ming
 * @LastEditTime: 2021-12-03 21:33:51
 */
#ifndef _MONITOR_H
#define _MONITOR_H

#define BUFFSIZE 512
#define WAIT_SECOND 1
#define MB 1024

#define DEBUG 0

typedef struct _CPU_PACKED {
    char name[16];
    unsigned int user;   //用户模式
    unsigned int nice;   //低优先级的用户模式
    unsigned int system; //内核模式
    unsigned int idle;   //空闲处理器时间
} CPU_OCCUPY;

/*sysinfo系统相关信息的结构体*/
struct sysinfo info;

/*主机的状态信息结构体*/
typedef struct _HOST_STATE {
    int hour;
    int minute;
    double cpu_used;
    double mem_used;
} HOST_STATE;

/*收发数据包结构体*/
typedef struct _RTX_BYTES {
    long int tx_bytes;
    long int rx_bytes;
    struct timeval rtx_time;
} RTX_BYTES;

/*网卡设备信息结构体*/
typedef struct _NET_INTERFACE {
    char name[16];  /*网络接口名称*/
    char ip[16];    /*网口IP*/
    double d_speed; /*下行速度*/
    double u_speed; /*上行速度*/
    char mac[13];   /*网口MAC地址*/
    /*上下行速度级别 bit 7~0
     *bit[0]=d_speed
     *bit[1]=u_speed
     *1:MB/s 0:KB/s
     */
    unsigned char speed_level; /**/
    RTX_BYTES rtx0_cnt;
    RTX_BYTES rtx1_cnt;
    struct _NET_INTERFACE *next; /*链表指针*/
} NET_INTERFACE;

/**
 * @description: 必须先打开sysinfo文件，读取内存和运行时间信息
 * @param {*}
 * @return {*}
 * @author: Ming
 */
void open_sysinfo();

/**
 * @description: 获取终端运行时间
 * @param {int} *hours
 * @param {int} *minutes
 * @return {*}
 * @author: Ming
 */
void get_host_runtime(int *hours, int *minutes);

/**
 * @description: 获取某个时刻的CPU负载
 * @param {CPU_OCCUPY} *cpu
 * @return {*}
 * @author: Ming
 */
void get_cpuoccupy(CPU_OCCUPY *cpu);

/**
 * @description: 计算CPU占用率
 * @param {CPU_OCCUPY} *c1
 * @param {CPU_OCCUPY} *c2
 * @return {*}
 * @author: Ming
 */
double cal_occupy(CPU_OCCUPY *c1, CPU_OCCUPY *c2);

/**
 * @description: 获取内存占用率
 * @param {double} *mem_used
 * @return {*}
 */
void get_mem_usage(double *mem_used);

/**
 * @description: 获取内存占用率
 * @param {double} *cpu_used
 * @return {*}
 * @author: Ming
 */
void get_cpu_usage(double *cpu_used);

/**
 * @description: 打开网络接口设备文件/proc/net/dev
 * @param {FILE} *
 * @return {*}
 */
void open_netconf(FILE **fd);

/**
 * @description: 获取网卡数量和IP地址
 * @param {NET_INTERFACE} *
 * @param {int} *n 本机网卡数量
 * @return {*}
 */
int get_interface_info(NET_INTERFACE **net, int *n);

/**
 * @description: 显示本机活动网络接口
 * @param {NET_INTERFACE} *p_net
 * @return {*}
 */
void show_netinterfaces(NET_INTERFACE *p_net, int n);

/**
 * @description: 获取网卡当前时刻的收发包信息
 * @param {char} *name
 * @param {RTX_BYTES} *rtx
 * @return {*}
 */
void get_rtx_bytes(char *name, RTX_BYTES *rtx);

/**
 * @description: rtx结构体拷贝--disable
 * @param {RTX_BYTES} *dest
 * @param {RTX_BYTES} *src
 * @return {*}
 */
void rtx_bytes_copy(RTX_BYTES *dest, RTX_BYTES *src);

/**
 * @description: 计算网卡的上下行网速
 * @param {double} *u_speed
 * @param {double} *d_speed
 * @param {unsignedchar} *level
 * @param {RTX_BYTES} *rtx0
 * @param {RTX_BYTES} *rtx1
 * @return {*}
 */
void cal_netinterface_speed(double *u_speed, double *d_speed,
                            unsigned char *level, RTX_BYTES *rtx0,
                            RTX_BYTES *rtx1);

/**
 * @description: 获取主机网卡速度信息
 * @param {NET_INTERFACE} *p_net
 * @return {*}
 */
void get_network_speed(NET_INTERFACE *p_net);

/**
 * @description: 网络信息监控线程
 * @param {*}
 * @return {*}
 */
void *thread_net(void *arg);

/**
 * @description: CPU、内存、运行时间监控线程
 * @param {*}
 * @return {*}
 */
void *thread_core(void *arg);

#endif