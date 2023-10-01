#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <sstream>

#include <map>

int debug = 1;
bool interrupt_disable = false;

std::map<int, const char*> NumToOpcode;

void init_num_to_opcode() {
   NumToOpcode[1] = "Load value";
   NumToOpcode[2] = "Load addr";
   NumToOpcode[3] = "LoadInd addr";
   NumToOpcode[4] = "LoadIdxX addr";
   NumToOpcode[5] = "LoadIdxY addr";
   NumToOpcode[6] = "LoadSpX";
   NumToOpcode[7] = "Store addr";
   NumToOpcode[8] = "Get";
   NumToOpcode[9] = "Put port";
   NumToOpcode[10] = "AddX";
   NumToOpcode[11] = "AddY";
   NumToOpcode[12] = "SubX";
   NumToOpcode[13] = "SubY";
   NumToOpcode[14] = "CopyToX";
   NumToOpcode[15] = "CopyFromX";
   NumToOpcode[16] = "CopyToY";
   NumToOpcode[17] = "CopyFromY";
   NumToOpcode[18] = "CopyToSp";
   NumToOpcode[19] = "CopyFromSp";
   NumToOpcode[20] = "Jump addr";
   NumToOpcode[21] = "JumpIfEqual addr";
   NumToOpcode[22] = "JumpIfNotEqual addr";
   NumToOpcode[23] = "Call addr";
   NumToOpcode[24] = "Ret";
   NumToOpcode[25] = "IncX";
   NumToOpcode[26] = "DecX";
   NumToOpcode[27] = "Push";
   NumToOpcode[28] = "Pop";
   NumToOpcode[29] = "Int";
   NumToOpcode[30] = "IRet";
   NumToOpcode[50] = "End";
}

int read_memory(int write_fd, int read_fd, int location) {
   if (location > 999 && !interrupt_disable) {
      throw std::out_of_range("location out of user range");
   }

   std::string write_packet;
   write_packet = "0 " + std::to_string(location) + "\n";
   if(debug > 100){
      std::cout << write_packet << std::endl;
   }
   write(write_fd, &write_packet, write_packet.size());
   char read_char;
   std::string read_packet;
   while (true) {
      read(read_fd, &read_char, 1);
      if (read_char == '\n') {
         break;
      }
      read_packet += read_char;
   }
   if(debug > 100){
      std::cout << read_packet << std::endl;
   }
   std::istringstream read_stream(read_packet);
   std::string temp;
   read_stream >> temp;
   if (std::stoi(temp) == 0) {
      printf("failed read");
   }
   else {
      read_stream >> temp;
   }
   return std::stoi(temp);
}

void write_memory(int write_fd, int read_fd, int location, int data) {
   if(location > 999 && !interrupt_disable){
      throw std::out_of_range("location out of user range");
   }
   std::string write_packet;
   write_packet = "1 " + std::to_string(location) + " " + std::to_string(data) + "\n";
   if(debug > 100){
      std::cout << write_packet << std::endl;
   }
   write(write_fd, &write_packet, write_packet.size());
   char read_char;
   std::string read_packet;
   while (true) {
      read(read_fd, &read_char, 1);
      if (read_char == '\n') {
         break;
      }
      read_packet += read_char;
   }
   if(debug > 100){
      std::cout << read_packet << std::endl;
   }
   std::istringstream read_stream(read_packet);
   std::string temp;
   read_stream >> temp;
   if (std::stoi(temp) == 0) {
      printf("failed write");
   }
}

void exit_memory(int write_fd) {
   std::string write_packet = "2";
   write(write_fd, &write_packet, write_packet.size());
}


void spawn_memory(int &write_pipe, int &read_pipe, const std::string& file_name) {
   int pipe_fd_CPU[2], pipe_fd_memory[2];
   int pipe_status_CPU, pipe_status_memory;
   pipe_status_CPU = pipe(pipe_fd_CPU);
   if (pipe_status_CPU != 0) {
      printf("Failed CPU pipe");
      exit(1);
   }
   pipe_status_memory = pipe(pipe_fd_memory);
   if (pipe_status_memory != 0) {
      printf("Failed memory pipe");
      exit(1);
   }

   int pid = fork();
   if (pid == 0) {
      //Memory
      // making cin and cout the read and write pipes
      close(0);
      dup2(pipe_fd_memory[0], 0);
      close(1);
      dup2(pipe_fd_CPU[1], 1);
      close(pipe_fd_CPU[0]);
      close(pipe_fd_memory[1]);
      int exec_return = execl("memory", "memory", file_name.c_str(), NULL);
      // should never reach
      std::cerr << "failed to start memory subsystem. exec return = " << exec_return << ", errno = "
                << errno << std::endl;
      exit(1);
   }
   close(pipe_fd_CPU[1]);
   close(pipe_fd_memory[0]);
   write_pipe = pipe_fd_memory[1];
   read_pipe = pipe_fd_CPU[0];
}

void cpu_instructions(int write_fd, int read_fd, int timer_interval) {
   int PC = 0;
   int IR = 0;
   int AC = 0;
   int X = 0;
   int Y = 0;
   int SP = 999;
   int timer_count = 0;
   std::string memory_request;
   while (true) {
      if(timer_count >= timer_interval){
         if(!interrupt_disable){
            if(debug > 20){
               std::cerr << "invoking timer interrupt" << std::endl;
            }
            interrupt_disable = true;
            int SP_user = SP;
            SP = 1999;
            write_memory(write_fd, read_fd, SP, SP_user);
            SP--;
            write_memory(write_fd, read_fd, SP, PC);
            SP--;
            PC = 1000;
            continue;
         }
      }
      timer_count++;
      IR = read_memory(write_fd, read_fd, PC);
      if(debug > 10){
         const char * opname = "ERR";
         if (NumToOpcode.count(IR)) {
            opname = NumToOpcode[IR];
         }
         std::cerr <<"PC = " <<  PC << ", IR = " << IR << "(" << opname << ")" << ", AC = " << AC << ", X = " << X << ", Y = " << Y << ", SP = " << SP << ", cycle = "<< timer_count << std::endl;
      }
      if(X > 25){

      }
      switch (IR) {
         case 1:
            PC++;
            AC = read_memory(write_fd, read_fd, PC);
            break;

         case 2: {

            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr);
            break;

         }
         case 3: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            int indirect_addr = read_memory(write_fd, read_fd, addr);
            AC = read_memory(write_fd, read_fd, indirect_addr);
            break;
         }

         case 4: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr + X);
            break;
         }

         case 5: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr + Y);
            break;
         }

         case 6: {
            AC = read_memory(write_fd, read_fd, SP + X);
            break;
         }

         case 7: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            write_memory(write_fd, read_fd, addr, AC);
            break;
         }

         case 8: {
            AC = (rand() % 100) + 1;
            break;
         }

         case 9: {
            PC++;
            int port = read_memory(write_fd, read_fd, PC);
            if (port == 1) {
               std::cout << AC;
            }
            else {
               std::cout << char(AC);
            }
            std::cout.flush();
            break;
         }

         case 10: {
            AC += X;
            break;
         }

         case 11: {
            AC += Y;
            break;
         }

         case 12: {
            AC -= X;
            break;
         }

         case 13: {
            AC -= Y;
            break;
         }

         case 14: {
            X = AC;
            break;
         }

         case 15: {
            AC = X;
            break;
         }

         case 16: {
            Y = AC;
            break;
         }

         case 17: {
            AC = Y;
            break;
         }

         case 18: {
            SP = AC;
            break;
         }

         case 19: {
            AC = SP;
            break;
         }

         case 20: {
            PC++;
            PC = read_memory(write_fd, read_fd, PC);
            continue;
         }

         case 21: {
            PC++;
            if (AC == 0) {
               PC = read_memory(write_fd, read_fd, PC);
               continue;
            }
            break;
         }

         case 22: {
            PC++;
            if (AC != 0) {
               PC = read_memory(write_fd, read_fd, PC);
               continue;
            }
            break;
         }

         case 23: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            PC++;
            SP--;
            write_memory(write_fd, read_fd, SP, PC);
            PC = addr;
            continue;
         }

         case 24: {
            PC = read_memory(write_fd, read_fd, SP);
            SP++;
            continue;
         }

         case 25: {
            X++;
            break;
         }

         case 26: {
            X--;
            break;
         }

         case 27: {
            SP--;
            write_memory(write_fd, read_fd, SP, AC);
            break;
         }

         case 28: {
            AC = read_memory(write_fd, read_fd, SP);
            SP++;
            break;
         }

         case 29: {
            if(interrupt_disable){
               throw std::runtime_error("int called while interrupt active");
            }
            interrupt_disable = true;
            int SP_user = SP;
            SP = 1999;
            write_memory(write_fd, read_fd, SP, SP_user);
            PC++;
            SP--;
            write_memory(write_fd, read_fd, SP, PC);
            PC = 1500;
            SP--;
            continue;
         }

         case 30: {

            SP++;
            PC = read_memory(write_fd, read_fd, SP);
            SP++;
            SP = read_memory(write_fd, read_fd, SP);
            interrupt_disable = false;
            timer_count = 0;
            continue;
         }

         case 50: {
            exit_memory(write_fd);
            if(debug > 10){
               std::cout << "exit processed" << std::endl;
            }
            exit(0);
         }
      }
      PC++;

   }
}

int main(int argc, char *argv[]) {
   init_num_to_opcode();
   // get arguments
   std::string file_name = argv[1];
   int timer = std::stoi(argv[2]);

   int write_fd;
   int read_fd;
   spawn_memory(write_fd, read_fd, file_name);
   try{
      cpu_instructions(write_fd, read_fd, timer);
   }
   catch(const std::out_of_range &exc){
      std::cout << "terminating with: " << exc.what() << std::endl;
   }
   catch(const std::runtime_error &exc){
      std::cout << "terminating with: " << exc.what() << std::endl;
   }


   //process
   //CPU

/*
// testing readfile
int memory[2000];
std::string file_name = "sample1.txt";
readFile(file_name, memory);
*/

   return 0;

}


