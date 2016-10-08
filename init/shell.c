/*
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <libc.h>
#include <fs.h>
#include <stdarg.h>
#include <string.h>

#define LINE_MAX 512

static int ret = 0;
static char pwd[FS_NAME_MAX * 10] = "/";

static int cmdls(char *line);
static int cmdcd(char *line);
static int cmdpwd(char *line);

struct cmd {
  char *name;
  int (*func)(char *);
};

struct cmd cmds[] = {
  { "ls",     &cmdls },
  { "cd",     &cmdcd },
  { "pwd",    &cmdpwd },
  { nil, nil },
};

int
cmdls(char *line)
{
  uint8_t buf[FS_NAME_MAX+2], len;
  struct stat s;
  char *filename;
  int r, fd;

  filename = strtok(nil, " ");

  if (filename == nil) {
    filename = ".";
  }

  if (stat(filename, &s) != OK) {
    printf("Error statting '%s'\n", filename);
    return ERR;
  }

  if (!(s.attr & ATTR_dir)) {
    printf("%b %u %s\n", s.attr, s.size, filename);
    return OK;
  }

  if (chdir(filename) != OK) {
    printf("Failed to descend into '%s'\n", filename);
    return ERR;
  }
  
  fd = open(".", O_RDONLY);
  if (fd < 0) {
    printf("Error opening '%s'\n", filename);
    return ERR;
  }

  while ((r = read(fd, &len, sizeof(len))) > 0) {
    if (read(fd, buf, len) <= 0) {
      break;
    }

    buf[len] = 0;

    if (stat((const char *) buf, &s) != OK) {
      printf("stat error %s\n", buf);
      continue;
    }

    printf("%b %u %s\n", s.attr, s.size, buf);
  }

  close(fd);

  chdir(pwd);
  
  return OK;
}

int
cmdcd(char *line)
{
  char *dir;

  dir = strtok(nil, " ");
  if (dir == nil) {
    dir = "/";
  }

  if (chdir(dir) != OK) {
    printf("Failed to change to '%s'\n", dir);
    return ERR;
  }

  memmove(pwd, ".", 2);
  cleanpath(pwd, pwd+1, sizeof(pwd));
  pwd[0] = '/';
  
  return 0;
}

int
cmdpwd(char *line)
{
  printf("%s\n", pwd);
  return OK;
}

static int
readline(uint8_t *data, size_t max)
{
  size_t i;
  char c;

  i = 0;
  while (i < max) {
    if (read(stdin, &c, sizeof(char)) < 0) {
      return -1;
    } else if (c == '\n') {
      data[i] = '\0';
      return i;
    } else {
      data[i++] = c;
    }
  }

  data[i-1] = 0;
  return i;
}

static void
processline(char *line)
{
  char *cmd;
  int i;

  cmd = strtok(line, " ");

  i = 0;
  while (cmds[i].name) {
    if (strcmp(cmds[i].name, cmd)) {
      ret = cmds[i].func(line);
      break;
    }

    i++;
  }

  if (cmds[i].name == nil) {
    printf("%s: command not found\n", cmd);
  }
}

int
shell(void)
{
  uint8_t line[LINE_MAX];

  while (true) {
    printf("%% ");
    readline(line, LINE_MAX);
    processline((char *) line);
  }

  return 0;
}
