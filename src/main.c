#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
  printf("%s [flags]\n",argv[0]);
  printf("\t-n Creates a new file\n");
  printf("\t-f <filepath> (required) The filepath of the database\n");
  printf("\t-h Shows this message\n");
}

int main(int argc, char *argv[]) { 
  char *filepath = NULL;
  bool newfile = false;
  
  int c=0;
  while((c = getopt(argc,argv,"nf:h")) != -1){
    switch (c) {
      case 'n':
        newfile = true;
        break;
      case 'f':
        filepath = optarg;
        break;
      case 'h':
        print_usage(argv);
        return 0;
      case '?':
        printf("Unknown option-%c\n",c);
        break;
      default:
        return -1;
    }
  }
  if(filepath==NULL){
    printf("Filepath is required\n");
    print_usage(argv);
    return -1;
  }
  int database_fd = newfile?create_db_file(filepath):open_db_file(filepath);
  if(database_fd == STATUS_ERROR){
    printf("Unable to access database file\n");
    return -1;
  }
  struct dbheader_t *header = NULL;
  if((newfile?create_db_header(database_fd,&header):validate_db_header(database_fd,&header))==STATUS_ERROR){
    printf("Error processing database header");
    return -1;
  }
  if(header == NULL){
    printf("Error, header is null");
    return -1;
  }
  if(output_file(database_fd,header,NULL) == STATUS_ERROR){
    printf("Error saving database to file");
    return -1;
  }
  return 0;
}
