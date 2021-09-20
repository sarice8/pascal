
/*
*****************************************************************************
*
*   Schema Browser
*
*   by Steve Rice
*
*   May 17, 1994
*
*****************************************************************************
*
*   browser.c           Schema database browser utility
*
*   This object should be linked with the object generated for a schema,
*   and schema_db.o.
*
*        e.g.    cc -o my_browser browser.o schema_db.o my_schema.o
*
*   HISTORY
*----------------------------------------------------------------------------
*   05/17/94 | Steve  | First version
*
*****************************************************************************
*/


#include <stdio.h>
#include <string.h>

#ifdef AMIGA
#include <dos.h>
#define bzero(p,n)   memset(p,'\0',n)
#endif /* AMIGA */


#include "schema_db.h"


/*
 *  Schema table access primitives, defined in <schema>_schema.c
 */

extern short    dGetAttributeOffset (/* class, attribute */);
extern short    dGetAttributeTags   (/* class, attribute */);
extern short    dGetAttributeType   (/* attribute */);

extern short    dObjectSize [];
extern int      dObjects;
extern char    *dObjectName [];
extern int      dAttributes;
extern char    *dAttributeName [];
extern short    dAttributeType [];


Node SCH_LookupNode (/* node_num */);


char      filename[256];
Node      Root;

/*
 *  browser context stack (use 'b' or 'back' to go back up the stack
 */

typedef struct context_struct *Context;

struct context_struct {
    Node       node;
    Context    next;
};

Context    current_context;


/*
 *  main ()
 *
 */

int     main (argc, argv)
int           argc;
char         *argv[];
{
    int    arg;

    if (argc != 2)
    {
        printf ("Usage:   browser <ascii_database_file>\n");
        exit (1);
    }

    arg = 1;
    strcpy (filename, argv[arg]);

    SCH_Init ();
    Root = SCH_LoadAscii (filename);

    if (Root == NULL)
    {
        printf ("Unable to read database %s\n", filename);
        exit (1);
    }

    browse (Root);
}

browse (Root)
Node    Root;
{
    char      command_line[256];
    char      command[256];
    int       num;
    Node      N;

    current_context = (Context) malloc(sizeof(struct context_struct));
    current_context->node = Root;
    current_context->next = NULL;


    while (1)
    {
        DumpNodeLong (current_context->node);

        while (1)
        {
            printf ("? ");
            gets (command_line);
            sscanf (command_line, "%s", command);

            if (strcmp(command, "help") == 0)
            {
                printf ("<n>:    go to node <n>\n");
                printf ("b:      back to previous node\n");
                printf ("quit:   quit browser\n");
            }
            else if (strcmp(command, "quit") == 0)
            {
                br_quit (0);
            }
            else if ((strcmp(command, "b") == 0) ||
                     (strcmp(command, "back") == 0))
            {
                if (current_context->next == NULL)
                {
                    printf ("No previous node.\n");
                }
                else
                {
                    Context   t;
                    t = current_context;
                    current_context = current_context->next;
                    free (t);
                    break;
                }
            }
            else
            {
                num = atoi (command);
                if (num == 0)
                {
                    printf ("Unknown command; type help for help\n");
                }
                else
                {
                    Context   t;
                    for (t = current_context; t != NULL; t = t->next)
                        if (t->node->node_num == num)
                            break;

                    if (t)
                    {
                        /* Returning to a node we've been to.  Delete all between then & now */
                        while (current_context->node->node_num != num)
                        {
                            t = current_context;
                            current_context = current_context->next;
                            free (t);
                        }
                    }
                    else
                    {
                        N = SCH_LookupNode (num);
                        if (N == NULL)
                        {
                            printf ("Node %d does not exist in the database\n", num);
                            continue;
                        }

                        t = (Context) malloc(sizeof(struct context_struct));
                        t->node = N;
                        t->next = current_context;
                        current_context = t;
                    }

                    break;
                }
            }
        }
    }
}


br_quit (status)
int      status;
{
    exit (status);
}

