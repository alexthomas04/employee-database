#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC 0x4c4c4144

struct dbheader_t {
	unsigned int magic;
	unsigned short version;
	unsigned short count;
	unsigned int filesize;
};

struct __attribute__((packed)) employee_t {
	char name[256];
	char address[256];
	unsigned int hours;
};

int create_db_header(struct dbheader_t **headerOut);
int validate_db_header(FILE *fp, struct dbheader_t **headerOut);
int read_employees(FILE *fp, struct dbheader_t *, struct employee_t **employeesOut);
int output_file(FILE *fp, struct dbheader_t *, struct employee_t *employees);
void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees);
int parse_str_employee( struct employee_t *employee, char *addstring);
int write_employee(FILE *fp, struct employee_t *e);
size_t employee_to_buffer(char *buffer, struct employee_t *e);
int get_employee_net_size(struct employee_t *employee);
void parse_net_employee(char *buffer, struct employee_t *employee);
int write_header(FILE *fp,struct dbheader_t *dbhdr);
#endif
