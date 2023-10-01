#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <sstream>

#include <map>

int debug = 0;
bool interrupt_disable = false;

std::map<int, const char *> NumToOpcode;

// used for debugging
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

// takes in file descriptors and where to read and returns value
int read_memory(int write_fd, int read_fd, int location) {
   // keeping user mode out of system memory
   if (location > 999 && !interrupt_disable) {
      throw std::out_of_range("location out of user range");
   }

   std::string write_packet;
   // first int identifies it as a read or write. Second int is the location
   write_packet = "0 " + std::to_string(location) + "\n";
   // debugging tool
   if (debug > 500) {
      std::cerr << "write packet = " << write_packet << std::endl;
   }
   // sends packet to memory
   write(write_fd, write_packet.c_str(), write_packet.size());
   // reads received packet
   char read_char;
   std::string read_packet;
   // gets packet char by char until it sees \n
   while (true) {
      read(read_fd, &read_char, 1);
      if (read_char == '\n') {
         break;
      }
      read_packet += read_char;
   }
   // debugging tool
   if (debug > 500) {
      std::cerr << "read packet = " << read_packet << std::endl;
   }
   // puts received packet into an string stream
   std::istringstream read_stream(read_packet);
   std::string temp;
   // first int indicates success or failure
   read_stream >> temp;
   if (std::stoi(temp) == 0) {
      printf("failed read");
   }
   else {
      // second int is the value in memory
      read_stream >> temp;
   }
   return std::stoi(temp);
}

// writes to memory takes file descriptors, index and data
void write_memory(int write_fd, int read_fd, int location, int data) {
   // keeping user mode out of system memory
   if (location > 999 && !interrupt_disable) {
      throw std::out_of_range("location out of user range");
   }
   std::string write_packet;
   // first int 1 means write. Second int is the location. third int is the new data
   write_packet = "1 " + std::to_string(location) + " " + std::to_string(data) + "\n";
   // debug tool
   if (debug > 500) {
      std::cerr << "write packet = " << write_packet << std::endl;
   }
   // send packet
   write(write_fd, write_packet.c_str(), write_packet.size());
   char read_char;
   std::string read_packet;
   // reads packet char by char until it sees a \n
   while (true) {
      read(read_fd, &read_char, 1);
      if (read_char == '\n') {
         break;
      }
      read_packet += read_char;
   }
   // debug tool
   if (debug > 500) {
      std::cerr << "read packet = " << read_packet << std::endl;
   }
   std::istringstream read_stream(read_packet);
   std::string temp;
   // only 1 int returned 1 meaning success and 0 meaning failure
   read_stream >> temp;
   if (std::stoi(temp) == 0) {
      printf("failed write");
   }
}

// exit function takes file descriptor to memory
void exit_memory(int write_fd) {
   // one int in write packet 2 meaning exit
   std::string write_packet = "2";
   write(write_fd, write_packet.c_str(), write_packet.size());
}


// creating memory pipe fork exec
void spawn_memory(int &write_pipe, int &read_pipe, const std::string &file_name) {
   int pipe_fd_CPU[2], pipe_fd_memory[2];
   int pipe_status_CPU, pipe_status_memory;
   // creating pipes and checking for failed creation
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

   // forking and starting up child process
   int pid = fork();
   if (pid == 0) {
      //Memory
      // child process will be memory
      // closing cin and cout and replacing them with the pipe read and write file descriptors
      close(0);
      dup2(pipe_fd_memory[0], 0);
      close(1);
      dup2(pipe_fd_CPU[1], 1);
      //closing unneeded file descriptors
      close(pipe_fd_CPU[0]);
      close(pipe_fd_memory[1]);
      // execing and starting the new memory process, passing in file name
      int exec_return = execl("memory", "memory", file_name.c_str(), NULL);
      // should never reach throws error if it does
      std::cerr << "failed to start memory subsystem. exec return = " << exec_return << ", errno = "
                << errno << std::endl;
      exit(1);
   }
   // closing unneeded file descriptors for parent process with is the CPU
   close(pipe_fd_CPU[1]);
   close(pipe_fd_memory[0]);
   // assigning useful file descriptors making it easier to read
   write_pipe = pipe_fd_memory[1];
   read_pipe = pipe_fd_CPU[0];
}

// main loop running through all the instructions
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
      // interrupt checking
      timer_count++;
      if ((timer_count % timer_interval) == 0) {
         // making sure not already in a interrupt
         if (!interrupt_disable) {
            // debugging tool
            if (debug > 20) {
               std::cerr << "invoking timer interrupt" << std::endl;
            }
            // saving off SP and PC and setting it to kernel mode
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
         // resetting the timer count
         timer_count = 0;
      }
      // reading new instruction
      IR = read_memory(write_fd, read_fd, PC);
      // debugging tool
      if (debug > 10) {
         const char *opname = "ERR";
         if (NumToOpcode.count(IR)) {
            opname = NumToOpcode[IR];
         }
         std::cerr << "PC = " << PC << ", IR = " << IR << "(" << opname << ")" << ", AC = " << AC
                   << ", X = " << X << ", Y = " << Y << ", SP = " << SP << ", cycle = "
                   << timer_count << std::endl;
      }
      // all cases
      switch (IR) {
         // load value
         case 1: {
            PC++;
            AC = read_memory(write_fd, read_fd, PC);
            break;
         }

            // load addr
         case 2: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr);
            break;
         }

            // load indirect addr
         case 3: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            int indirect_addr = read_memory(write_fd, read_fd, addr);
            AC = read_memory(write_fd, read_fd, indirect_addr);
            break;
         }

            // load addr + X
         case 4: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr + X);
            break;
         }

            // load addr + Y
         case 5: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr + Y);
            break;
         }

            // load SP + X
         case 6: {
            AC = read_memory(write_fd, read_fd, SP + X);
            break;
         }

            // store addr
         case 7: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            write_memory(write_fd, read_fd, addr, AC);
            break;
         }

            // get random number 1 - 100
         case 8: {
            AC = (rand() % 100) + 1;
            break;
         }

            // write to screen 1 = int 2 = char
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

            // add X to AC
         case 10: {
            AC += X;
            break;
         }

            // add Y to AC
         case 11: {
            AC += Y;
            break;
         }

            // subtract X from AC
         case 12: {
            AC -= X;
            break;
         }

            // subtract Y from AC
         case 13: {
            AC -= Y;
            break;
         }

            // copy AC to X
         case 14: {
            X = AC;
            break;
         }

            // copy X to AC
         case 15: {
            AC = X;
            break;
         }

            // copy AC to Y
         case 16: {
            Y = AC;
            break;
         }

            // copy Y to AC
         case 17: {
            AC = Y;
            break;
         }

            // copy AC to SP
         case 18: {
            SP = AC;
            break;
         }

            // copy SP to AC
         case 19: {
            AC = SP;
            break;
         }

            // jump to addr
         case 20: {
            PC++;
            PC = read_memory(write_fd, read_fd, PC);
            continue;
         }

            // jump to addr if AC = 0
         case 21: {
            PC++;
            if (AC == 0) {
               PC = read_memory(write_fd, read_fd, PC);
               continue;
            }
            break;
         }

            // jump to addr if AC != 0
         case 22: {
            PC++;
            if (AC != 0) {
               PC = read_memory(write_fd, read_fd, PC);
               continue;
            }
            break;
         }

            // push addr to stack
         case 23: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            PC++;
            SP--;
            write_memory(write_fd, read_fd, SP, PC);
            PC = addr;
            continue;
         }

            // pop addr from stack and jump to addr
         case 24: {
            PC = read_memory(write_fd, read_fd, SP);
            SP++;
            continue;
         }

            // X++
         case 25: {
            X++;
            break;
         }

            // X--
         case 26: {
            X--;
            break;
         }

            // push AC onto stack
         case 27: {
            SP--;
            write_memory(write_fd, read_fd, SP, AC);
            break;
         }

            // pop AC off of stack
         case 28: {
            AC = read_memory(write_fd, read_fd, SP);
            SP++;
            break;
         }

            // call interrupt turn to kernel mode and start executing code at 1500. save SP and PC;
         case 29: {
            if (interrupt_disable) {
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

            // return from interrupt. restore SP and PC exit kernel mode and return to user mode
         case 30: {
            SP++;
            PC = read_memory(write_fd, read_fd, SP);
            SP++;
            SP = read_memory(write_fd, read_fd, SP);
            interrupt_disable = false;
            continue;
         }

            // exit and sends exit to memory
         case 50: {
            exit_memory(write_fd);
            if (debug > 10) {
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
   //start memory
   spawn_memory(write_fd, read_fd, file_name);
   // run switch case
   try {
      cpu_instructions(write_fd, read_fd, timer);
   }
   catch (const std::out_of_range &exc) {
      std::cout << "terminating with: " << exc.what() << std::endl;
   }
   catch (const std::runtime_error &exc) {
      std::cout << "terminating with: " << exc.what() << std::endl;
   }

   return 0;

}


