
// Test: Can I still emit code to heap memory?
// Or do I need to protect it somehow.

// NOTES
// Some random article about registers in x86
//   https://stackoverflow.com/questions/36529449/why-are-rbp-and-rsp-called-general-purpose-registers
//   Wikipedia has some info  https://en.wikipedia.org/wiki/X86


#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
//  -- for mmap, mprotect
#include <errno.h>

typedef void (*funcptr)();

char* codeMem = 0;
size_t codeMemLength = 1024;
char* pc = 0;

void
outB( char c )
{
  *(pc++) = c;
}

void
outI( int i )
{
  int* p = (int*) pc;
  *(p++) = i;
  pc = (char*) p;
}



int
main( int argc, char* argv[] )
{
   // codeMem = malloc(codeMemLength);
   codeMem = mmap( NULL, codeMemLength, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );

   pc = codeMem;

   outB( 0x55 );  // push %rbp
   outB( 0x48 );  // mov  %rsp,%rbp
   outB( 0x89 );
   outB( 0xe5 );
   outB( 0x48 );  // sub $0x40,%rsp
   outB( 0x83 );
   outB( 0xec );
   outB( 0x40 );

   outB( 0xc9 );  // leaveq
   outB( 0xc3 );  // retq

#if 0
   // Old opcodes from very old experiment, which was 32-bit.
   // I'll need to figure out 64-bit instruction set.

   // start stack frame

   outB( 0x55 );  // push ebp       -- Now is push rbp
   outB( 0x8b );  // mov ebp, esp   -- Need to switch to rsp, rbp.  Getting SEGV on this.
   outB( 0xec );
   outB( 0x81 );  // sub esp, (int) 20
   outB( 0xec );
   outI( 20 );
   outB( 0x56 );  // push esi
   outB( 0x57 );  // push edi

   outB( 0x8d );  // lea esi, [ebp-(int)20]
   outB( 0xb5 );
   outI( -20 );

   
   // end stack frame
   outB( 0x5f );  // pop edi
   outB( 0x5e );  // pop esi
   outB( 0xc9 );  // leave

   // ret
   outB( 0xc3 );  // ret
#endif

   // Switch memory from writable to executable.
   // This also ensures the instruction cache doesn't have stale data.
   if ( mprotect( codeMem, codeMemLength, PROT_READ | PROT_EXEC ) != 0 ) {
     printf( "error: mprotect failed with status %d\n", errno );
     exit( 1 );
   }

   printf( "Here we go!\n" );

   funcptr f = (funcptr) codeMem;
   f();

   printf( "Done\n" );
}


// Here's a bit of assembly from start of main of sm.c
// Note gdb is printing in AT&T syntax (since unix heritage) i.e.   "move from, to"
// as opposed to the Intel syntax prevelant in windows world i.e.  "move to, from"

// => 0x555555555269 <main>:	endbr64 
//    0x55555555526d <main+4>:	push   %rbp
//    0x55555555526e <main+5>:	mov    %rsp,%rbp
//    0x555555555271 <main+8>:	sub    $0x40,%rsp
//    0x555555555275 <main+12>:	mov    %edi,-0x34(%rbp)
//    0x555555555278 <main+15>:	mov    %rsi,-0x40(%rbp)
//    0x55555555527c <main+19>:	mov    %fs:0x28,%rax
//    0x555555555285 <main+28>:	mov    %rax,-0x8(%rbp)
//    0x555555555289 <main+32>:	xor    %eax,%eax
//    0x55555555528b <main+34>:	movb   $0x0,0x1d49e(%rip)        # 0x555555572730 <trace>
//    0x555555555292 <main+41>:	movl   $0x1,-0x18(%rbp)
//    0x555555555299 <main+48>:	mov    -0x18(%rbp),%eax
//    0x55555555529c <main+51>:	cmp    -0x34(%rbp),%eax
//    0x55555555529f <main+54>:	jl     0x5555555553a7 <main+318>
//    0x5555555552a5 <main+60>:	lea    0x2d5c(%rip),%rdi        # 0x555555558008
//    0x5555555552ac <main+67>:	callq  0x555555555100 <puts@plt>
//    0x5555555552b1 <main+72>:	mov    $0xffffffff,%edi
//    0x5555555552b6 <main+77>:	callq  0x555555555170 <exit@plt>
// 


