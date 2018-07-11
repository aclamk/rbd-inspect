#ifndef PARSE_LOG_H
#define PARSE_LOG_H

#include <string>

typedef enum {op_read=1, op_write=2} op_dir;
typedef enum {op_unknown=0, op_seq=1, op_rand=2} op_type;

struct op_io_t {
  double tv; //time
  uint32_t client_no = 0;
  op_dir opcode;
  std::string object_name;
  uint32_t offset;
  uint32_t len;
};

struct op_mode_t {
  op_dir opcode;
  op_type type;
};

bool parse_line(const std::string& line, uint32_t& client_no, std::string& object_name, struct op_io_t& specifics);
#endif
