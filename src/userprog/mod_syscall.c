#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/*mod_include****************************************************************/
/****************************************************************************/


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


bool valid(void *vaddr){
  ; 
}


/*mod_macro******************************************************************/
#define STK_PTR		esp		// the name of stack pointer
#define IS_VALID	valid		// the name of pointer vaildation function
#define HANDLE 		thread_exit()	// handle invalid pointer

// stack pointers for argments
#define ARG1_AMONG_1 (STK_PTR + 1)

#define ARG1_AMONG_2 (STK_PTR + 4)
#define ARG2_AMONG_2 (STK_PTR + 5)

#define ARG1_AMONG_3 (STK_PTR + 5)
#define ARG2_AMONG_3 (STK_PTR + 6)
#define ARG3_AMONG_3 (STK_PTR + 7)


// validate pointer for each case
//#define VALIDATE_0ARGS
#define VALIDATE_1ARG	(!IS_VALID(ARG1_AMONG_1))?HANDLE:NULL;
#define VALIDATE_2ARGS	(!(IS_VALID(ARG1_AMONG_2)&&IS_VALID(ARG2_AMONG_2)))?HANDLE:NULL;
#define VALIDATE_3ARGS	(!(IS_VALID(ARG1_AMONG_3)&&IS_VALID(ARG2_AMONG_3)&&IS_VALID(ARG3_AMONG_3)))?HANDLE:NULL;
/****************************************************************************/


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t *esp = f->esp;  

  // check the validity of stack pointer to syscall number
  if(!valid(esp)){
    thread_exit();
  }

  int syscall_enum = *(int *)(f->esp);

  // check the validity of stack pointer to arguments
  switch(syscall_enum){
  // 0 arguments
  case SYS_HALT:
    break;
  // 1 argument
  case SYS_EXIT:
    VALIDATE_1ARG; break; 
  case SYS_EXEC:
    VALIDATE_1ARG; break; 
  case SYS_WAIT:
    VALIDATE_1ARG; break; 
  case SYS_REMOVE:
    VALIDATE_1ARG; break; 
  case SYS_OPEN:
    VALIDATE_1ARG; break; 
  case SYS_FILESIZE:
    VALIDATE_1ARG; break; 
  case SYS_TELL:
    VALIDATE_1ARG; break; 
  case SYS_CLOSE:
    VALIDATE_1ARG; break; 
  // 2 arguments
  case SYS_CREATE:
    VALIDATE_2ARGS; break; 
  case SYS_SEEK:
    VALIDATE_2ARGS; break; 
  // 3 arguments
  case SYS_READ:
    VALIDATE_3ARGS; break; 
  case SYS_WRITE: 
    VALIDATE_3ARGS; break; 
  // error
  default:
    thread_exit(); break;
  }

  printf ("system call!\n");
  thread_exit();
}

