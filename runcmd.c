/* runcmd.c - Execute a command as a subprocess.

   Copyright (c) 2016 Emanuel Valente <emanuelvalente@gmail.com>

   Nutshell is derived from POSIXeg Fool Shell -
   https://gitlab.com/monaco/posixeg/shell/foolsh
   Copyright (c) 2014 Francisco Jose Monaco
   POSIXeg repository can be found at https://gitlab.com/monaco/posixeg

   This file is part of Nutshell
   Nutshell is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <runcmd.h>
#include <debug.h>

void signal_from_child_handler(int signal, siginfo_t* info, void *u);
int is_nonblock_array[MAX_PID_VALUE];

/* Executes 'command' in a subprocess. Information on the subprocess execution
   is stored in 'result' after its completion, and can be inspected with the
   aid of macros made available for this purpose. Argument 'io' is a pointer
   to an integer vector where the first, second and third positions store
   file descriptors to where standard input, output and error, respective,
   shall be redirected; if NULL, no redirection is performed. On
   success, returns subprocess' pid; on error, returns 0. */

int runcmd (const char *command, int *result,  int *io) /* ToDO: const char* */
{
  int pid, status, pipeid[2];
  int aux, total_args, i, tmp_result, is_nonblock;
  char *args[RCMD_MAXARGS], *p, *cmd, buff;
  struct sigaction act;


  tmp_result = 0;
  is_nonblock = 0;


  /* Parse arguments to obtain an argv vector. */

  cmd = malloc ((strlen (command)+1) * sizeof(char));
  sysfail (!cmd, -1);
  p = strcpy (cmd, command);

  total_args = 0;
  args[total_args++] = strtok (cmd, RCMD_DELIM);
  while ((total_args<RCMD_MAXARGS) && (args[total_args++] = strtok (NULL, RCMD_DELIM)));
  total_args--;
  args[total_args] = NULL;

  /*verifies if it will execute as non-blocking mode*/
  if(args[total_args -1][0] == '&')
   {
      is_nonblock = 1;
      tmp_result |= NONBLOCK;
      /*overwrites '&'*/
      args[total_args -1] = NULL;
    }

  /*using pipe to discover if there was error of
  * execution: ex: ./foo (=wont set EXECOK)*/
  pipe(pipeid);

  /*clean signal handlers*/
  if(! memset(&act, 0, sizeof(struct sigaction)) )
    sysfail(1, 0);


  /* Create a subprocess. */
  pid = fork();
  sysfail (pid<0, -1);

  if (IS_NONBLOCK(tmp_result) && runcmd_onexit != NULL)
    {
      struct sigaction sa_runcmd;
      sa_runcmd.sa_sigaction = signal_from_child_handler;
      sysfail(sigaction(SIGCHLD, &sa_runcmd, NULL) < 0, -1);
    }


  if (pid>0)			/* Caller process (parent). */
    {

      if(!is_nonblock)
        {
          aux = waitpid(pid, &status, 0);
          /*aux = wait (&status);*/
          sysfail (aux<0, -1);

          /*if aux has one byte -> EXECOK
          * otherwhise (has two bytes) -> ./foo
          * won't set EXECOK*/
          close(pipeid[1]); /*we don't want to write*/
          aux = read(pipeid[0], &buff, 2);

         /* Collect termination mode. */
          if (WIFEXITED(status) && aux == 1)
           {
              tmp_result |= EXECOK;
              tmp_result |= NORMTERM;
              tmp_result |= (WEXITSTATUS(status) & RETSTATUS) ;
            }
         /*at this point (./fooo (no such file) !EXECOK
         * we just want to set exit returned value*/
          else
              tmp_result |= (WEXITSTATUS(status) & RETSTATUS) ;
      }
    }
  else /* Subprocess (child) */
    {
      /*verifies if we should redirect I/O*/

      if(io)
        {
          int std_fd[]={ STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
          for (i=0; i<3; i++)
            {
              if (io[std_fd[i]] > 0)
                {
                  aux = dup2(io[std_fd[i]], std_fd[i]);
                  sysfail (aux<0, -1);
                }
            }
        }

      /*write one byte on pipe*/
      close(pipeid[0]); /*we don't want to read*/
      buff = 0;
      write(pipeid[1], &buff, 1);

      if(is_nonblock)
        /*fix me*/
        setpgid(0, 0);

      aux = execvp (args[0], args);

      /*only reaches here if an error occured:
       * writes on more byte (error)
       * on pipe (error occurred)*/
      buff = 1;
      write(pipeid[1], &buff, 1);

      free (cmd);
      close(pipeid[1]);
      exit (EXECFAILSTATUS);
    }

  if (result)
    *result = tmp_result;

  free (p);
  close(pipeid[0]); /*parent*/
  return pid;			/* Only parent reaches this point. */
}

/* If function runcmd is called in non-blocking mode, then function
   pointed by rcmd_onexit is asynchronously evoked upon the subprocess
   termination. If this variable points to NULL, no action is performed.
*/



void (*runcmd_onexit)(void) = NULL;

void signal_from_child_handler(int sig, siginfo_t *info, void *u)
{
	if (runcmd_onexit != NULL)
		runcmd_onexit();
}


