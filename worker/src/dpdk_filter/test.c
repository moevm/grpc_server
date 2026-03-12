#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile int force_quit = 0;

static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\nSignal %d received, exiting...\n", signum);
        force_quit = 1;
    }
}

int main(int argc, char **argv) {
    int ret;
    uint16_t nb_ports;
    struct rte_mempool *mbuf_pool;
    struct rte_mbuf *pkts[32];
    
    // Установка обработчика сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Инициализация EAL - ВСЕ АРГУМЕНТЫ ПЕРЕДАЮТСЯ СЮДА
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        printf("EAL init failed\n");
        return -1;
    }
    
    // Получаем количество доступных портов
    nb_ports = rte_eth_dev_count_avail();
    printf("Number of available ports: %u\n", nb_ports);
    
    if (nb_ports == 0) {
        printf("No ports available. Did you pass --vdev argument?\n");
        rte_eal_cleanup();
        return -1;
    }
    
    // Используем первый порт
    uint16_t port_id = 0;
    printf("Using port %u\n", port_id);
    
    // Создание пула буферов
    mbuf_pool = rte_pktmbuf_pool_create("POOL", 8192, 250, 0, 
                                        RTE_MBUF_DEFAULT_BUF_SIZE, 
                                        rte_socket_id());
    if (!mbuf_pool) {
        printf("Failed to create mbuf pool\n");
        rte_eal_cleanup();
        return -1;
    }
    
    // Конфигурация порта
    struct rte_eth_conf port_conf = {0};
    ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret < 0) {
        printf("Failed to configure port: %s\n", strerror(-ret));
        rte_eal_cleanup();
        return -1;
    }
    
    // Настройка RX очереди
    ret = rte_eth_rx_queue_setup(port_id, 0, 1024, 
                                  rte_eth_dev_socket_id(port_id), 
                                  NULL, mbuf_pool);
    if (ret < 0) {
        printf("Failed to setup RX queue: %s\n", strerror(-ret));
        rte_eal_cleanup();
        return -1;
    }
    
    // Настройка TX очереди
    ret = rte_eth_tx_queue_setup(port_id, 0, 1024, 
                                  rte_eth_dev_socket_id(port_id), 
                                  NULL);
    if (ret < 0) {
        printf("Failed to setup TX queue: %s\n", strerror(-ret));
        rte_eal_cleanup();
        return -1;
    }
    
    // Запуск порта
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        printf("Failed to start port: %s\n", strerror(-ret));
        rte_eal_cleanup();
        return -1;
    }
    
    rte_eth_promiscuous_enable(port_id);
    
    printf("Started forwarding on port %u. Press Ctrl+C to exit.\n", port_id);
    
    // Основной цикл
    while (!force_quit) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, pkts, 32);
        
        if (nb_rx > 0) {
            printf("Received %u packets\n", nb_rx);
            for (int i = 0; i < nb_rx; i++) {
                rte_pktmbuf_free(pkts[i]);
            }
        }
        
        usleep(1000);
    }
    
    // Очистка
    printf("Cleaning up...\n");
    rte_eth_dev_stop(port_id);
    rte_eth_dev_close(port_id);
    rte_eal_cleanup();
    
    return 0;
}