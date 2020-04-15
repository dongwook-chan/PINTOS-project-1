#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/*mod_include****************************************************************/
#include "devices/shutdown.h"
/****************************************************************************/

/*mod_structure**************************************************************/
static void syscall_handler (struct intr_frame *);

typedef uint32_t (*func_of_1arg) (uint32_t arg1);
typedef uint32_t (*func_of_2arg) (uint32_t arg1, uint32_t arg2);
typedef uint32_t (*func_of_3arg) (uint32_t arg1, uint32_t arg2, uint32_t arg3);

struct list file_list;

typedef struct{
  int fd;
  struct file *file;
  struct list_elem elem;
}file_elem;

/****************************************************************************/

// array of function addresses indexed unsing syscall enumeratior
func_of_3arg syscall_arr[SYS_INUMBER + 1] =	// SYS_INMUBER is the last element in the enum
{ 
  // project 2
  (func_of_3arg)halt,
  (func_of_3arg)exit,
  (func_of_3arg)exec,
  (func_of_3arg)wait,
  (func_of_3arg)create,
  (func_of_3arg)remove,
  (func_of_3arg)open,
  (func_of_3arg)filesize,
  (func_of_3arg)read,
  (func_of_3arg)write,
  (func_of_3arg)seek,
  (func_of_3arg)tell,
  (func_of_3arg)close,

  // project 3
  /*
  (func_of_3arg)mmap,
  (func_of_3arg)munmap,
  */
  // project 4
  /*
  (func_of_3arg)chdir,
  (func_of_3arg)mkdir,
  (func_of_3arg)readdir,
  (func_of_3arg)isdir,
  (func_of_3arg)inumber,*/
  //(func_of_3arg),
};

// array of number of arguments indexed using syscal enumerator
char syscall_argc[SYS_INUMBER + 1] =
{
  0, // halt
  1, // exit
  1, // exec
  1, // wait
  2, // create
  1, // remove
  1, // open
  1, // filesize
  3, // read
  3, // write
  2, // seek
  1, // tell
  1  // close
};

bool syscall_ret[SYS_INUMBER + 1] =
{
  0, // halt
  0, // exit
  1, // exec
  1, // wait
  1, // create
  1, // remove
  1, // open
  1, // filesize
  1, // read
  1, // write
  0, // seek
  1, // tell
  0  // close
};

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  list_init(&file_list);
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

  int syscall_enum = *esp;    

  switch(syscall_argc[syscall_enum]){
  // 0 arguments
  case 0:
    break;

  // 1 argument
  case 1:
    // validate 1 argument
    if(!valid(ARG1_AMONG_1))
      thread_exit();
    
    if(syscall_ret[syscall_enum])
      *eax = ((func_of_1arg)syscall_arr[syscall_enum])(ARG1_AMONG_1);
    break;
  // 2 arguments
  case 2:
    // validate 2 arguments
    if(!(valid(ARG1_AMONG_2) && valid(ARG2_AMONG_2)))
      thread_exit();

    if(syscall_ret[syscall_enum])
      *eax = ((func_of_2arg)syscall_arr[syscall_enum])(ARG1_AMONG_2, ARG2_AMONG_2);
    break;
  // 3 arguments
  case 3:
    // validate 3 arguments
    if(!(valid(ARG1_AMONG_3) && valid(ARG2_AMONG_3) && valid(ARG3_AMONG_3)))
      thread_exit();
  
    if(syscall_ret[syscall_enum])
      *eax = (/*(func_of_3arg)*/syscall_arr[syscall_enum])(ARG1_AMONG_3, ARG2_AMONG_3, ARG3_AMONG_3);
    break;
  // error
  default:
    thread_exit();
  }
}

struct file *get_file_by_fd(struct list list, int fd){
  struct list_elem *e;
  file_elem *f;

  for (e = list_begin (&list); e != list_end(&list); e = list_next(e)){
    f = list_entry(e, file_elem, elem);
    if(f->fd == fd)
      return f->file;
  }
}



// syscall
//	1. pointer arguments must be verified with valid()

/* Terminates Pintos by calling shutdown_Power_off(). */
void halt (void){
  shutdown_power_off();
}

/* Terminates the current user program, returning status to the kernel. */
void exit (int status){
  thread_exit();
  /* If the process’s parent waits for it, this is the status that will be returned. */
    
}

pid_t exec (const char *cmd_line){
  return 0;
}

int wait (pid_t pid){
  return 0;
}

bool create (const char *file, unsigned initial_size){
  return 0;
}

bool remove (const char *file){
  return 0;
}

int open (const char *file){
  return 0;
}

int filesize (int fd){
  return 0;
}

int read (int fd, void *buffer, unsigned size){
  unsigned i;
  char c;
  uint8_t *cast_buffer = (uint8_t *)buffer;

  switch(fd){
  case STDIN_FILENO:

    for(i = 0; i < size && (c = input_getc()) != NULL ; ++i)
      cast_buffer[i] = (uint8_t)c;
    return i;
  case STDOUT_FILENO:
    return 0;
  default:
    return 0;
  }
}

int write (int fd, const void *buffer, unsigned size){
  switch(fd){
  case STDIN_FILENO:
    return 0;
  case STDOUT_FILENO:
    return 0;
  default:
    return 0;
  } 

  return 0;
}

void seek (int fd, unsigned position){
  ;
}

unsigned tell (int fd){
  return 0;
}

void close (int fd){
  ;
}

