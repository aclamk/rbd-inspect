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
#include <assert.h>

#include "parse_log.h"
#include "model.h"

int record(std::istream& input, std::ostream& output)
{
  std::map<uint32_t, Recorder_t> Recorders;

  std::string line;
  std::set<std::string> objects;
  while (getline(input, line))
  {
    uint32_t client_no;
    std::string object_name;
    struct op_io_t specifics;

    if ( !parse_line(line, client_no, object_name, specifics) )
      continue;
     Recorders[client_no].record(specifics);
  }
  for (auto &r: Recorders) {
    r.second.save(output);
  }
  return 0;

}

void print_help()
{
  static const char help[] =
      "Usage: rbd-record [OPTION]...\n"
      "Extract RBD operations from logs into model format.\n"
      "\n"
      " -i FILE         Read operations from rbd log FILE\n"
      " -o FILE         Write model to FILE\n"
      " -a | --append   Append, instead of overwrite\n"
      " -h, --help      Help\n";
  std::cout << help << std::endl;
}

int main(int argc, char** argv)
{
  std::string input_name = "";
  std::string output_name = "";
  bool append = false;
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
    if (arg == "-a" || arg == "--append") {
      append = true;
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
    foutput.open(output_name.c_str(), append ? std::ofstream::binary | std::ofstream::app : std::ofstream::binary);
    if (!foutput.good()) {
      std::cerr << "Cannot open '" << output_name << "' for writing" << std::endl;
      return false;
    }
    output = &foutput;
  }

  int result;
  result = record(*input, *output);
  return result;
}

