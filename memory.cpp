#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

int debug = 100;

void readFile(const std::string &file_name, int memory[]) {
   if (debug > 10) {
      std::cerr << "Reading file '" << file_name << "'" << std::endl;
   }
   std::ifstream file(file_name);
   if (!file.is_open()) {
      std::cout << "Failed to open File" << std::endl;
      return;
   }
   int index = 0;
   std::string line;
   while (getline(file, line)) {
      std::string opcode;
      // check for memory position change
      if (line[0] == '.') {
         // check for comments after
         int x = line.find(' ');
         if (line.find(' ') > -1) {
            index = std::stoi(line.substr(1, line.find(' ')));
         }
         else {
            index = std::stoi(line.substr(1, line.size()));
         }
         continue;
      }
         // check for empty line
      else if (line.empty()) {

      }
         // should be data or opcode at this point and will save it off into memory and ignore
      else if (line.find(' ')) {
         opcode = line.substr(0, line.find(' '));
         //memory location
         memory[index] = std::stoi(opcode);
      }
      else {
         memory[index] = std::stoi(line);
      }
      index++;
   }
   file.close();
}

int main(int argc, char *argv[]) {
   if(argc < 2){
      printf("no argument");
      exit(1);
   }
   std::string file_name = argv[1];
   int memory[2000];
   memset(memory, 0, sizeof(memory));
   readFile(file_name, memory);
   std::string input_string;
   while (getline(std::cin, input_string)) {
      // getting type of operation and memory location
      std::string output;
      std::istringstream input_stream(input_string);
      std::string temp;
      input_stream >> temp;
      int operation = std::stoi(temp);
      input_stream >> temp;
      int location = std::stoi(temp);

      if(operation == 2){
         exit(1);
      }
      // checking location
      else if (location < 0 || location > 2000) {
         output = "0";

      }

      // 0 is a read and 1 is a write operation
      else if (operation == 0) {
         // formatting line
         output = "1 " + std::to_string(memory[location]);
         if(debug > 10){
            std::cerr << "reading location - "<< location << " data - " << memory[location] << std::endl;
         }
      }
      else if (operation == 1) {
         // getting data
         input_stream >> temp;
         memory[location] = std::stoi(temp);
         output = "1 " + temp;
         if(debug > 10){
            std::cerr << "writing location - " << location << " data - " << std::stoi(temp) << std::endl;
         }
      }
      else {
         output = "0";
      }
      std::cout << output << std::endl;
   }
   return 0;
}