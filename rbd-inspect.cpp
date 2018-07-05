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
#include <assert.h>

#include "parse_log.h"

#include "model.h"

int main(int argc, char** argv)
{
  Model_t M;
  std::map<uint32_t, Model_t> Models;
  std::map<uint32_t, Learn_t> Learns;

  std::ifstream rbd_log( argv[1] );
  if (!rbd_log.is_open()) {
    std::cerr << "Cannot open " << argv[1] << std::endl;
  }
  std::string line;
  while (getline(rbd_log, line))
  {
    uint32_t client_no;
    std::string object_name;
    struct op_io_t specifics;

    if ( !parse_line(line, client_no, object_name, specifics) )
	continue;
    //std::cout << line.substr(0,26) << std::endl;
    std::cout << std::fixed << std::setprecision(6) << specifics.tv <<
	" " << client_no << " " << specifics.object_name << " " << specifics.opcode << " " << specifics.offset << " " << specifics.len << "." << std::endl;

    Learns[client_no].learn_object_op(Models[client_no], specifics);
  }

  for (auto &x: Models)
  {
    std::cout << std::endl << x.first << std::endl;//": ";
    x.second.r_w_print();
  }
  /*
  while (xxx-- > 0)
  {
    if (M.r_w_model() == op_read)
      std::cout << "R" << std::endl;
    else
      std::cout << "W" << std::endl;
  }
  */
}
