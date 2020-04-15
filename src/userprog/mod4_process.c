#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) 
    thread_exit ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED) 
{
  // return -1 for the below cases
  // terminated by the kernel
  // invalid TID || not child of calling proc || process_wait() already called for TID

  // exit value?
  // tid validity test || parent->child 검사? || ?

  while(1){
    ;
  }

  return -1;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };


/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

/***********************************************************************/

#define PUSH(buff_ptr, item, size)	{size_t tmp_size = size;			\
					memcpy(buff_ptr, item, tmp_size);	\
					buff_ptr += tmp_size;}

#define MEM_PUSH(item, size)	{size_t tmp_size = size; printf("var dec %d\n", tmp_size);					\
				memcpy(*esp, item, tmp_size); printf("memcpy %d\n", tmp_size);				\
				*esp = (uint8_t *)*esp + tmp_size; printf("esp mod\n");} 


#define MEM_SET(value,size) memset(*esp, value, size); *esp = (uint8_t *)*esp - size;

#define MAX_ARG 5
#define MAX_ARG_ADDR MAX_ARG * sizeof(char *)
#define MAX_FILENAME 10


  void strtok_m (char *s, const char *delimiters, char *argv[MAX_ARG], int *argc, size_t *arg_len){
	printf("The first character of the given string is %s NULL\n", (*(s-1)=='\0')?"":"NOT");
	printf("The givin string is : %s\n", s);

	*argc = 0;
	*arg_len = 0;

	s = s + strlen(s + 1);

	while (*s != '\0')
	{
		/* Skip any DELIMITERS at our current position. */
		// 공백 제거
		while (strchr(delimiters, *s) != NULL)
			--s;

		// 마지막 공백에 단어의 마지막임을 표시
		s[1] = '\0';	//

		/* Skip any non-DELIMITERS up to the end of the string. */
		// 단어 skip
		while (strchr(delimiters, *s) == NULL && *s != '\0'){
			putchar(*s);
			--s;
		}
		printf("\ncurrent character : %c, it is %s NULL\n", *s, (*s == '\0')?"":"NOT");

		// 마지막 문자가 단어의 처음임을 표시
		argv[(*argc)++] = s + 1;
		(*arg_len) += strlen(argv[(*argc) - 1]);
		printf("tokenized to argv : %s\n", argv[(*argc) - 1]);
	}
	printf("token success\n");
	return;
  }


 char *
 strtok_l (char *s, const char *delimiters, char **save_ptr)
 {

	char *token;

	printf("given string : %s\n", s);

	/* If S is nonnull, start from it.
	If S is null, start from saved position. */
	if (s != NULL)
		s += strlen(s) - 1;
	else
		s = *save_ptr;
	printf("last character : %c\n", *s);
	
	/* Skip any DELIMITERS at our current position. */
	// 공백 제거
	while (strchr(delimiters, *s) != NULL)
	{
		/* strchr() will always return nonnull if we're searching
		for a null byte, because every string contains a null
		byte (at the end). */
		if (*s == '\0')
		{
			*save_ptr = s;
			return NULL;
		}

		--s;
	}
	// 마지막 공백에 단어의 마지막임을 표시
	s[1] = '\0';	//

	/* Skip any non-DELIMITERS up to the end of the string. */
	// 단어 skip
	while (strchr(delimiters, *s) == NULL)
		--s;

	// 마지막 문자가 단어의 처음임을 표시
	token = s + 1;
	printf("token created : %s\n", token);

	// 문장의 마지막이 아니라면
	if (*s != '\0')
		// 다음 수행 때는 공백 하나를 skip한 지점부터 시작
		*save_ptr = s - 1;
	// 문장의 마지막에 도달했다면
	else
		*save_ptr = s;

	return token;

 }




/***********************************************************************/
static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofset;
  bool success = false;
  int i;



/***********************************************************************/
 
  char tmp_file_name[MAX_FILENAME];
  char *token, *save_ptr;
  int argc;
  int esp_offset_argv;
  int esp_offset_addr;
  char addr_list[MAX_ARG_ADDR + 1];
  char *addr_ptr = addr_list;
  int excess;
  uint8_t *temp_esp;
  int tmp_size;
  char *argv[MAX_ARG];
  size_t arg_len;

/***********************************************************************/


  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /* Open executable file. */
/***********************************************************************/
  memcpy(tmp_file_name, file_name, strlen(file_name) + 1);
  file = filesys_open (strtok_r(tmp_file_name, " \n\r\t\0", &save_ptr));
  printf("tmp_file_name : %s\n", tmp_file_name);
/***********************************************************************/
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofset = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofset < 0 || file_ofset > file_length (file))
        goto done;
      file_seek (file, file_ofset);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofset += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;





/**********************************************************************/
   memcpy(tmp_file_name + 1, file_name, strlen(file_name) + 1);
   printf("tmp_file_name : %s\n", tmp_file_name);
   tmp_file_name[0] = '\0';
   printf("tmp_file_name : %s\n", tmp_file_name);

   strtok_m(tmp_file_name, " \r\v\n", argv, &argc, &arg_len);
   printf("escape token\n");
   printf("argc : %darg_len : %d\n", argc, arg_len);

   printf("esp : %x\n", *esp);
   esp_offset_argv = arg_len + agrc;
   esp_offset_addr = sizeof(unit8_t) + (argc + 1) * sizeof(char *) + sizeof(char **) + sizeof(int) + sizeof(void*);
   printf("%d\n", esp_offset_argv);
   *esp = *esp - esp_offset_argv;
   printf("esp : %x\n", *esp);
   printf("esp modified\n");
  //hex_dump(*esp,*esp,128 ,true);
   // tokenize and increment argc
   for(i = 0; i < argc; ++i){
           printf("arg to push : %s, length : %d\n", argv[i], strlen(argv[i] ));
   printf("esp : %x\n", *esp);
	   // push argv[i]
           MEM_PUSH(argv[i], strlen(argv[i]) + 1);

	   printf("token pushed : %s\n", argv[i]);
  //hex_dump(*esp,*esp,128 ,true);

           // create argv address list
           PUSH(addr_ptr, esp, sizeof(char *));
   }
   *addr_ptr = '\0';
   *esp = *esp - esp_offset_argv - esp_offset_addr;




putchar('\n');
  hex_dump(*esp,*esp,128 ,true);




   // word align
   excess = esp_offset % 4;
   if (excess){           // file_name 길이가 4바이트로 나눠 떨어지지 않으면
           MEM_SET(0, 4 - excess);
  }

   // push argv[-1]
   MEM_SET(0, sizeof(char *));

   // push &(argv[i])
   MEM_PUSH(addr_list, strlen(addr_ptr));

   // push argv
   temp_esp = *esp;
   MEM_PUSH(&temp_esp, sizeof(char **));

   // push arc
   MEM_PUSH(&argc, sizeof(int));

   // push ret addr
  MEM_SET(0, sizeof(void *));

  hex_dump(PHYS_BASE - 128,*esp,128,true);

/**********************************************************************/



  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;


  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *knpage = palloc_get_page (PAL_USER);
      if (knpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, knpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (knpage);
          return false; 
        }
      memset (knpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, knpage, writable)) 
        {
          palloc_free_page (knpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp) 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        *esp = PHYS_BASE;
      else
        palloc_free_page (kpage);
    }
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *th = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (th->pagedir, upage) == NULL
          && pagedir_set_page (th->pagedir, upage, kpage, writable));
}
