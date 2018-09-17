#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>
#include <ctime>
#include <map>
#include <vector>
#include <set>
#include <list>
#include <assert.h>

#include "parse_log.h"

int convert(std::istream& input, std::ostream& output)
{
  std::string line;
  typedef std::list<std::string> object_order_t;
  typedef std::map<std::string, object_order_t::iterator> object_map_t;

  object_order_t object_lru_order;
  object_map_t object_lru;
  std::set<std::string> objects;

  while (getline(input, line))
  {
    uint32_t client_no;
    std::string object_name;
    struct op_io_t specifics;

    if ( !parse_line(line, client_no, object_name, specifics) )
      continue;

    if (objects.count(specifics.object_name) == 0) {
      objects.emplace(specifics.object_name);
      output << specifics.object_name << " add" << std::endl;
    }

    if (object_lru.count(specifics.object_name) == 0) {
      auto n = object_lru.emplace(specifics.object_name, object_lru_order.end());
      assert(n.second);
      object_lru_order.push_front(specifics.object_name);
      n.first->second = object_lru_order.begin();
      if (object_lru_order.size() > 1000) {
        object_lru.erase(object_lru_order.back());
        output << object_lru_order.back() << " close" << std::endl;
        object_lru_order.pop_back();
      }
      output << specifics.object_name << " open" << std::endl;
    } else {
      object_lru_order.erase(object_lru[specifics.object_name]);
      object_lru_order.push_front(specifics.object_name);
      object_lru[specifics.object_name] = object_lru_order.begin();
    }
    output << specifics.object_name << " " << (specifics.opcode==op_write?"write":"read") <<
        " " << specifics.offset << " " << specifics.len << std::endl;
  }
  return 0;
}



void print_help()
{
  static const char help[] =
      "Usage: rbd-extract [OPTION]...\n"
      "Extract RBD operations from logs into fio's --read_iolog format.\n"
      "\n"
      " -i FILE         Read operations from rbd log FILE.\n"
      " -o FILE         Write iolog output to FILE\n"
      " -h, --help      Help\n";
  std::cout << help << std::endl;
}


int main(int argc, char** argv)
{
  std::string input_name = "";
  std::string output_name = "";

  argc--; argv++;
  std::set<std::string> oneparam_args={"-i", "-o"};
  while (argc >= 1)
  {
    std::string arg = *argv;
    argc--; argv++;

    if (oneparam_args.count(arg) > 0) {
      //one param argument
      if (argc < 1) {
        std::cerr << "Option " << arg << " requires parameter" << std::endl;
        exit(-1);
      }
      std::string value = *argv;
      if (arg == "-i") {
        input_name = value;
      }
      if (arg == "-o") {
        output_name = value;
      }
      argc--; argv++;
      continue;
    }
    if (arg == "-h" || arg == "--help") {
      print_help();
      return 0;
    }
    std::cerr << "Unknown option '" << arg << "'" << std::endl;
    print_help();
    return 1;
  }

  std::ifstream rbd_log;
  std::istream* input;
  if (input_name == "") {
    input = &std::cin;
  } else {
    rbd_log.open(input_name.c_str(), std::ifstream::binary);
    if (!rbd_log.good()) {
      std::cerr << "Cannot open '" << input_name << "' for writing" << std::endl;
      return false;
    }
    input = &rbd_log;
  }

  std::ofstream foutput;
  std::ostream* output;
  if (output_name == "") {
    output = &std::cout;
  } else {
    foutput.open(output_name.c_str(), std::ofstream::binary);
    if (!foutput.good()) {
      std::cerr << "Cannot open '" << output_name << "' for writing" << std::endl;
      return false;
    }
    output = &foutput;
  }

  int result;
  result = convert(*input, *output);
  return result;
}
