#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum paging_mode {
    LEGACY = 2,
    PAE = 3,
} paging_mode_t;

typedef enum error {
    SUCCESS = 0,
    PAGE_FAULT = -1,
    READ_FAULT = -2,
    INVALID_TRANSLATION_TYPE = -3,
} error_t;

// TODO: move to legacy.c/pae.c
enum flags {
    // PDE
    P_PDE4KB = 0,
    P_PDE2MB = 0,
    P_PDE4MB = 0,
    PS_PDE4KB = 7,
    PS_PDE2MB = 7,
    PS_PDE4MB = 7,

    // PTE
    P_PTE = 0,
    PAT_PTE = 7,
};

// чтение указанного количества байт физической памяти по заданному адресу в указанный буфер
// функция вернет количество прочитанных байт (меньшее или 0 означает ошибку - выход за пределы памяти)
typedef uint32_t (*pread_func_t)(void *buf, const uint32_t size, const uint64_t physical_addr);

typedef struct config {
    // paging mode
    paging_mode_t level;

    // cr3
    uint32_t root_addr;

    // function which reads from physical-address
    pread_func_t read_func;

    // page-size extensions for 32-bit paging
    bool pse;

    // page-size extensions with 40-bit physical-address extension
    bool pse36;

    // page-attribute table
    bool pat;

    // execute disable
    bool nxe;

    // supervisor-mode access prevention
    bool smap;

    // override smap
    bool ac;

    // supervisor-mode execution prevention
    bool smep;

    // allows pages to be protected from supervisor-mode writes
    bool wp;

    // true if cpl < 3
    bool supervisor;

    // physical-address width supported by the processor
    uint8_t maxphyaddr;
} config_t;


// функция трансляции виртуального адреса в физический:
//
// virt_addr - виртуальный адрес подлежащий трансляции
// level - количество уровней трансляции 2 или 3 (соответствуют legacy и PAE трансляциям в x86)
// root_addr - физический адрес корня дерева трансляции (соответствует регистру CR3)
// read_func - функция для чтения физической памяти (см. объявление выше)
// функция возвращает успешность трансляции: 0 - успешно, не 0 - ошибки
// phys_addr - выходной оттранслированный физический адрес
error_t
va2pa(uint32_t virt_addr, const config_t *cfg, uint64_t *phys_addr, uint32_t *page_fault);

