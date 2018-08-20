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

int main(int argc, char** argv)
{
  Model_t M;
  std::map<uint32_t, Model_t> Models;
  //std::map<uint32_t, Learn_t> Learns;
  std::map<uint32_t, Recorder_t> Recorders;

  std::ifstream rbd_log( argv[1] );
  if (!rbd_log.is_open()) {
    std::cerr << "Cannot open " << argv[1] << std::endl;
  }
  std::string line;
  std::set<std::string> objects;
  while (getline(rbd_log, line))
  {
    uint32_t client_no;
    std::string object_name;
    struct op_io_t specifics;

    if ( !parse_line(line, client_no, object_name, specifics) )
	continue;
    if (false) std::cout << std::fixed << std::setprecision(6) << specifics.tv <<
	" " << specifics.client_no << " " << specifics.object_name << " " << specifics.opcode << " " << specifics.offset << " " << specifics.len << "." << std::endl;

    //Learns[client_no].learn_object_op(Models[client_no], specifics);
    Recorders[client_no].record(specifics);
    //Models[client_no].r_w_print();
  }
  /*
  for (auto &l: Learns) {
    l.second.learn_end(Models[l.first]);
  }
  */

  FILE *f = fopen("test-record", "a+");
  for (auto &r: Recorders) {
    //r.second.printall();
    r.second.save(f);
  }
  fclose(f);
  //print_history();


  f = fopen("test-record", "r");

  while (true) {
    int ch = fgetc(f);
    if (feof(f)) break;
    ungetc(ch, f);
    Player_t p;
    assert(p.load(f));
    Player_t::entry e;
    while(p.pop_next(e)) {
      //std::cout << "TIME=" << e.msec << " obj=" << e.obj_id << " " << e.operation << " o=" << e.offset << " l=" << e.len << std::endl;
    }
  }
  fclose(f);
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
