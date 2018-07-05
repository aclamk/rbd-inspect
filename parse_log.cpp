#include "parse_log.h"
#include <string.h>

//2014-12-17 00:02:26.368617 7fc37e047700  1 -- 10.247.68.105:6802/18233 <== client.2901606 10.247.68.67:0/1053093 28684 ==== osd_op(client.2901606.0:6804035 rbd_data.29bf88245cc9e0.000000000000031c [stat,set-alloc-hint object_size 4194304 write_size 4194304,write 1093632~4096] 3.72c22d00 snapc a969=[a969,a41b,a27e] ack+ondisk+write e75292) v4 ==== 291+0+4096 (3713658618 0 3540319769) 0x1808d340 con 0xfbe9b80
//2014-12-16 14:16:13.167764 7fc37e047700  1 -- 10.247.68.105:6802/18233 <== client.2423467 10.247.68.73:0/1026808 4310 ==== osd_op(client.2423467.0:5687263 rbd_data.29207a2895b15.0000000000000566 [read 3248128~40960] 3.432b5541 ack+read e75146) v4 ==== 190+0+0 (4048465505 0 0) 0x27ed2f40 con 0x55f23c0

bool parse_line(const std::string& line, uint32_t& client_no, std::string& object_name, struct op_io_t& specifics)
{
  std::string client_str;
  std::string operation;
  std::string offset_str;
  std::string len_str;
  //01234567890123456789012345
  //2014-12-16 14:16:13.167764

  static time_t tt;
  static std::string prev_time;
  if (line.substr(0,19) != prev_time) {
    struct tm t = {0};
    t.tm_year = atoi(&line[0]) - 1900;
    t.tm_mon =  atoi(&line[5]) - 1;
    t.tm_mday = atoi(&line[8]);
    t.tm_hour = atoi(&line[11]);
    t.tm_min =  atoi(&line[14]);
    t.tm_sec =  atoi(&line[17]);
    tt = mktime(&t);
    prev_time = line.substr(0,19);
  }
  specifics.tv = tt + double(atoi(&line[20])) / 1000000;

  size_t p0;
  p0 = line.find("<== client.");
  if (p0 == std::string::npos)
    return false;
  size_t p1 = p0 + strlen("<== client.");
  size_t p2;
  p2 = line.find(" ", p1);
  if (p2 == std::string::npos)
    return false;

  client_str = line.substr(p1, p2 - p1);
  client_no = std::stoi(client_str);
  specifics.client_no = client_no;
  //std::cout << client_no << std::endl;

  size_t p3;
  p3 = line.find("==== osd_op(",p2);
  if (p3 == std::string::npos)
    return false;
  p3 += strlen("==== osd_op(");

  size_t p4;
  p4 = line.find(" ", p3);
  if (p4 == std::string::npos)
    return false;
  p4 += 1;

  size_t p5;
  p5 = line.find(" ", p4);
  if (p5 == std::string::npos)
    return false;

  object_name = line.substr(p4, p5 - p4);
  specifics.object_name = object_name;
  //std::cout << object_name << std::endl;
  p5 += 1;

  size_t p6;
  p6 = line.find("[", p5);
  if (p6 == std::string::npos)
    return false;
  p6 += 1;

  size_t p7;
  p7 = line.find("]", p6);
  if (p7 == std::string::npos)
    return false;

  operation = line.substr(p6, p7 - p6);
  //std::cout << operation << std::endl;

  size_t p8;
  p8 = operation.find("write ");
  if (p8 != std::string::npos) {
    specifics.opcode = op_write;
    p8 += strlen("write");
  } else {
    p8 = operation.find("read ");
    if (p8 != std::string::npos) {
	specifics.opcode = op_read;
	p8 += strlen("read");
    } else
	return false;
  }
  p8 += 1;
  size_t p9;
  p9 = operation.find("~", p8);
  if (p9 == std::string::npos)
    return false;

  offset_str = operation.substr(p8, p9 - p8);
  p9 += 1;
  len_str = operation.substr(p9);
  specifics.offset = std::stoi(offset_str);
  specifics.len = std::stoi(len_str);
  return true;
}
