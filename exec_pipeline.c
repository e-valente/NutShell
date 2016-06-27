#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <tparse.h>
#include <runcmd.h>
#include <native_commands.h>
#include <exec_pipeline.h>
#include <debug.h>

#define MAX_BUFF_COMMAND 512

void exec_pipeline_one_command(pipeline_t *pipeline)
{
  int i, j, result;
  char *cmd;

  /*prepares cmd buffer*/
  cmd = (char*)malloc(sizeof(char) * MAX_BUFF_COMMAND);

  strncpy(cmd, pipeline->command[0][0], MAX_BUFF_COMMAND);


  /*process args*/
  for (j=1; pipeline->command[0][j]; j++)
    {
      strncat(cmd, " ", MAX_BUFF_COMMAND);
      strncat(cmd, pipeline->command[0][j], MAX_BUFF_COMMAND);
    }

      runcmd(cmd, &result, NULL);
      if(!IS_EXECOK(result))
        fprintf(stderr, "NutShell: %s: command not found\n", cmd);
}

void exec_pipeline_redir_input(pipeline_t *pipeline, int index)
{
  int i, j, result, tmp, fd;
  char *cmd;
  int *io;

  io = (int*)malloc(sizeof(int) * 3);
  /*prepares cmd buffer*/
  cmd = (char*)malloc(sizeof(char) * MAX_BUFF_COMMAND);

  strncpy(cmd, pipeline->command[index][0], MAX_BUFF_COMMAND);


  /*process args*/
  for (j=1; pipeline->command[index][j]; j++)
    {
      strncat(cmd, " ", MAX_BUFF_COMMAND);
      strncat(cmd, pipeline->command[index][j], MAX_BUFF_COMMAND);
    }

  tmp = open(pipeline->file_in, O_RDONLY);
  fatal(tmp < 0, "Could not redirect input: file not found");
  io[0] = tmp;
  io[1] = STDOUT_FILENO;
  io[2] = STDERR_FILENO;

  runcmd(cmd, &result, io);

  if(!IS_EXECOK(result))
    fprintf(stderr, "NutShell: %s: command not found\n", cmd);

}


void exec_pipeline_redir_output(pipeline_t *pipeline, int index)
{
  int i, j, result, tmp, fd;
  char *cmd;
  int *io;

  io = (int*)malloc(sizeof(int) * 3);

  /*prepares cmd buffer*/
  cmd = (char*)malloc(sizeof(char) * MAX_BUFF_COMMAND);

  strncpy(cmd, pipeline->command[index][0], MAX_BUFF_COMMAND);


  /*process args*/
  for (j=1; pipeline->command[index][j]; j++)
    {
      strncat(cmd, " ", MAX_BUFF_COMMAND);
      strncat(cmd, pipeline->command[index][j], MAX_BUFF_COMMAND);
    }

  tmp = open(pipeline->file_out, O_CREAT | O_TRUNC | O_RDWR , S_IRUSR | S_IWUSR);
  fatal(tmp < 0, "Could not redirect output");
  io[0] = STDIN_FILENO;
  io[1] = tmp;
  io[2] = STDERR_FILENO;

  runcmd(cmd, &result, io);

  if(!IS_EXECOK(result))
    fprintf(stderr, "NutShell: %s: command not found\n", cmd);

}

void exec_pipeline_redir_input_output(pipeline_t *pipeline, int index)
{
  int i, j, result, tmp1, tmp2;
  char *cmd;
  int *io;

  io = (int*)malloc(sizeof(int) * 3);

  /*prepares cmd buffer*/
  cmd = (char*)malloc(sizeof(char) * MAX_BUFF_COMMAND);

  strncpy(cmd, pipeline->command[index][0], MAX_BUFF_COMMAND);


  /*process args*/
  for (j=1; pipeline->command[index][j]; j++)
    {
      strncat(cmd, " ", MAX_BUFF_COMMAND);
      strncat(cmd, pipeline->command[index][j], MAX_BUFF_COMMAND);
    }

  tmp1 = open(pipeline->file_in, O_RDONLY);
  fatal(tmp1 < 0, "Could not redirect input");
  tmp2 = open(pipeline->file_out, O_CREAT | O_TRUNC | O_RDWR , S_IRUSR | S_IWUSR);
  fatal(tmp2 < 0, "Could not redirect output");

  io[0] = tmp1;
  io[1] = tmp2;
  io[2] = STDERR_FILENO;

  runcmd(cmd, &result, io);

  if(!IS_EXECOK(result))
    fprintf(stderr, "NutShell: %s: command not found\n", cmd);

}


