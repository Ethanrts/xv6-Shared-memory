#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"



struct {

  struct spinlock lock;

  struct shm_page {

    uint id;

    char *frame;

    int refcnt;

  } shm_pages[64];

} shm_table;



void shminit() {

  int i;

  initlock(&(shm_table.lock), "SHM lock");

  acquire(&(shm_table.lock));

  for (i = 0; i< 64; i++) {

    shm_table.shm_pages[i].id =0;

    shm_table.shm_pages[i].frame =0;

    shm_table.shm_pages[i].refcnt =0;

  }

  release(&(shm_table.lock));

}



int shm_open(int id, char **pointer){

  acquire(&(shm_table.lock));
  uint sz = PGROUNDUP(myproc()->sz);
  uint pa;
  int i;
  int fg = 0;

  for (i = 0; i< 64; i++) {
    if(shm_table.shm_pages[i].id == id) {
      fg = 1;
      pa = V2P(shm_table.shm_pages[i].frame);
      //map it to the existing page (use mappages())
      mappages(myproc()->pgdir,(char*)sz, PGSIZE, pa, PTE_W|PTE_U);
      //increment the refcnt
      shm_table.shm_pages[i].refcnt++;
    }
  }
  for (i = 0; i< 64; i++) {
    if(fg == 0){
      if(shm_table.shm_pages[i].id == 0) {
        //find the index of first available page and assign id to the page (page.id = id)
        shm_table.shm_pages[i].id = id;
        //page.frame = kalloc()
        shm_table.shm_pages[i].frame = kalloc();
        //increment the refcnt
        shm_table.shm_pages[i].refcnt++;
        //Initialize the frame allocated to 0. (use memset())
        memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
        //map the new page (using mappages())
        pa = V2P(shm_table.shm_pages[i].frame);
        mappages(myproc()->pgdir,(char*)sz, PGSIZE, pa, PTE_W|PTE_U);
        break;
      }
    }
  } 
  //return the address of the page through the pointer variable. πointer = (char *)size;
  *pointer=(char*)myproc()->sz;
  //update current process size
  myproc()->sz += PGSIZE;
  //release lock
  release(&(shm_table.lock));
  return 0; 

}





int shm_close(int id){

  //acquire a lock on shm_tabl
  acquire(&(shm_table.lock));
  //loop through all 64 pages and search if there exists a page whose id is same as id given as an argument 
  for (int i = 0; i< 64; i++) {
    if(shm_table.shm_pages[i].id == id) {
    //if found, decrement the refcnt 
      shm_table.shm_pages[i].refcnt--;
      //if refcnt is = 0:
      if(shm_table.shm_pages[i].refcnt == 0) {
      //set all the values of the page back to 0
        shm_table.shm_pages[i].id = 0;
        shm_table.shm_pages[i].frame = 0;
        shm_table.shm_pages[i].refcnt = 0;
      }
    }
  }
  //release lock 
  release(&(shm_table.lock));
  return 0; 
}

