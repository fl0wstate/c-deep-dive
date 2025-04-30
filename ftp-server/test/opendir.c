#include "../ftp.h"
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  struct dirent *directoryInfo;
  DIR *pdire;
  char lpwd[BUFFSIZE];

  // also the executable is an argument
  if (argc == 1)
  {
    if (!getcwd(lpwd, BUFFSIZE))
      LOG(ERROR, "Current working directory failed");
    pdire = opendir(lpwd);
  }
  else if (argc > 0)
    pdire = opendir(argv[1]);

  if (!pdire)
    LOG(ERROR, "Failed to open the current working directory %s", lpwd);

  while ((directoryInfo = readdir(pdire)))
  {
    LOG(INFO, "%s %s",
        directoryInfo->d_type == 4   ? "DIR:"
        : directoryInfo->d_type == 8 ? "FILE:"
                                     : "UNDEF",
        directoryInfo->d_name);
  }

  closedir(pdire);
  return EXIT_SUCCESS;
}
