#pragma once

#define LEGACY  2
#define PAE     3

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define LINEAR_SIZE     (1UL << (PAGE_SHIFT + 10))

/* PMD_SHIFT determines the size of the area a second-level page table can map */
#define PMD_SHIFT    (PAGE_SHIFT + (PAGE_SHIFT-3))
#define PMD_SIZE     (1UL << PMD_SHIFT)
#define PMD_MASK     (~(PMD_SIZE-1))

/* PGDIR_SHIFT determines what a third-level page table entry can map */
#define PGDIR_SHIFT    (PAGE_SHIFT + 2*(PAGE_SHIFT-3))
#define PGDIR_SIZE     (1UL << PGDIR_SHIFT)
#define PGDIR_MASK     (~(PGDIR_SIZE-1))

// чтение указанного количества байт физической памяти по заданному адресу в указанный буфер
// функция вернет количество прочианных байт (меньшее или 0 означает ошибку - выход за пределы памяти)
typedef unsigned int (*PREAD_FUNC)(void *buf, const unsigned int size, const unsigned int physical_addr);

// функция трансляции виртуального адреса в физический:
//
// virt_addr - виртуальный адрес подлежащий трансляции
// level - количество уровней трансляции 2 или 3 (соответствуют legacy и PAE трансляциям в x86)
// root_addr - физический адрес корня дерева трансляции (соответствует регистру CR3)
// read_func - функция для чтения физической памяти (см. объявление выше)
// функция возвращает успешность трансляции: 0 - успешно, не 0 - ошибки
// phys_addr - выходной оттранслированный физический адрес
int
va2pa(unsigned int virt_addr,
      unsigned int level,
      unsigned int root_addr,
      PREAD_FUNC read_func,
      unsigned int *phys_addr);

