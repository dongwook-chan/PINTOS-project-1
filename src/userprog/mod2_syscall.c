#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/*mod_include****************************************************************/
/****************************************************************************/


static void syscall_handler (struct intr_frame *);

typedef uint32_t (*syscall_ptr) (uint32_t arg1, uint32_t arg2, uint32_t arg3);
syscall_ptr syscall_arr[SYS_INUMBER + 1] =	// SYS_INMUBER is the last element in the enum
{ 
  // project 2
  (syscall_ptr)halt,
  (syscall_ptr)exit,
  (syscall_ptr)exec,
  (syscall_ptr)wait,
  (syscall_ptr)create,
  (syscall_ptr)remove,
  (syscall_ptr)open,
  (syscall_ptr)filesize,
  (syscall_ptr)read,
  (syscall_ptr)write,
  (syscall_ptr)seek,
  (syscall_ptr)tell,
  (syscall_ptr)close,

  // project 3
  /*
  (syscall_ptr)mmap,
  (syscall_ptr)munmap,
  */
  // project 4
  /*
  (syscall_ptr)chdir,
  (syscall_ptr)mkdir,
  (syscall_ptr)readdir,
  (syscall_ptr)isdir,
  (syscall_ptr)inumber,*/
  //(syscall_ptr),


}

void
syscall_init (void) 
{

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  
}




/*mod_macro******************************************************************/
#define STK_PTR		esp		// the name of stack pointer
//#define IS_VALID	valid		// the name of pointer vaildation function
//#define HANDLE 		thread_exit()	// handle invalid pointer

// stack pointers for argments
#define ARG1_AMONG_1 (STK_PTR + 1)

#define ARG1_AMONG_2 (STK_PTR + 4)
#define ARG2_AMONG_2 (STK_PTR + 5)

#define ARG1_AMONG_3 (STK_PTR + 5)
#define ARG2_AMONG_3 (STK_PTR + 6)
#define ARG3_AMONG_3 (STK_PTR + 7)


// validate pointer for each case
//#define VALIDATE_0ARGS
//#define VALIDATE_1ARG	(!IS_VALID(ARG1_AMONG_1))?HANDLE:NULL;
//#define VALIDATE_2ARGS	(!(IS_VALID(ARG1_AMONG_2)&&IS_VALID(ARG2_AMONG_2)))?HANDLE:NULL;
//#define VALIDATE_3ARGS	(!(IS_VALID(ARG1_AMONG_3)&&IS_VALID(ARG2_AMONG_3)&&IS_VALID(ARG3_AMONG_3)))?HANDLE:NULL;
/****************************************************************************/

// function type for 3 types of functions
typedef uint32_t (*func_of_3arg) (uint32_t arg1, uint32_t arg2, uint32_t arg3);

bool valid(void *vaddr){
  return true; 
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t *esp = f->esp;
  uint32_t *eax = &(f->eax);

  // check the validity of stack pointer to syscall number
  if(!valid(esp)){
    thread_exit();
  }

  int syscall_enum = *(int *)esp;

  // check the validity of stack pointer to arguments
  switch(syscall_enum){
  // 0 arguments
  case SYS_HALT:
    halt(); break;

  // 1 argument
  case SYS_EXIT:
    VALIDATE_1ARG; 
    exit(*ARG1_AMONG_1); break;
  case SYS_EXEC:
    VALIDATE_1ARG; 
    f->eax = exec(*ARG1_AMONG_1); break;
  case SYS_WAIT:
    VALIDATE_1ARG; 
    f->eax = wait(*ARG1_AMONG_1); break;
  case SYS_REMOVE:
    VALIDATE_1ARG; 
    f->eax = remove(*ARG1_AMONG_1); break;
  case SYS_OPEN:
    VALIDATE_1ARG; 
    f->eax = open(*ARG1_AMONG_1); break;
  case SYS_FILESIZE:
    VALIDATE_1ARG; 
    f->eax = filesize(*ARG1_AMONG_1); break;
  case SYS_TELL:
    VALIDATE_1ARG; 
    f->eax = tell(*ARG1_AMONG_1); break;
  case SYS_CLOSE:
    VALIDATE_1ARG; 
    close(*ARG1_AMONG_1); break;

  // 2 arguments
  case SYS_CREATE:
    VALIDATE_2ARGS;
    f->eax = create(*ARG1_AMONG_2, *ARG2_AMONG_2); break;
  case SYS_SEEK:
    VALIDATE_2ARGS;
    seek(*ARG1_AMONG_2, *ARG2_AMONG_2); break;

  // 3 arguments
  case SYS_READ:
    VALIDATE_3ARGS;
    f->eax = read(*ARG1_AMONG_3, *ARG2_AMONG_3, *ARG3_AMONG_3); break;
  case SYS_WRITE: 
    VALIDATE_3ARGS;
    f->eax = write(*ARG1_AMONG_3, *ARG2_AMONG_3, *ARG3_AMONG_3); break;

  // error
  default:
    thread_exit(); break;
  }

  //printf ("system call!\n");
  thread_exit();
}

