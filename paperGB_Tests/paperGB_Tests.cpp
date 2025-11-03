#include "CppUnitTest.h"
#include "json.hpp"
#include <fstream>
#include <vector>
#include <iostream>

#include "cpu.h"
#include "gb.h"

using json = nlohmann::json;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace paperGBTests
{
	TEST_CLASS(timing_tests)
	{
	public:
		
		TEST_METHOD(opcode_timings)
		{	
			json optables_json;
			std::ifstream file("..\\..\\paperGB_Tests\\Opcodes.json", std::ifstream::binary);
		
			try
			{
				optables_json = json::parse(file);
			}
			catch (json::parse_error& ex)
			{
				std::cerr << "parse error at byte " << ex.byte << std::endl;
				Assert::Fail(L"Failed to parse Opcodes.json");
			}

			Cartridge* cart = new Cartridge();
			GB* gameboy = new GB(*cart);
			CPU cpu = CPU(gameboy);
			int start = 0;
			int end = 0;
			std::stringstream opcode_stream;
			std::wstringstream message;
			std::vector<int> cycles;

			
			for (int i = 0; i <= 0xFF; i++) {
				opcode_stream.clear();
				opcode_stream.str("");
				opcode_stream << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << i;
				std::string opcode = opcode_stream.str();
				
				message.clear();
				message.str(L"");
				message << L"Opcode: " << std::wstring(opcode.begin(), opcode.end());

				try
				{
					cycles = optables_json["unprefixed"][opcode]["cycles"].get<std::vector<int>>();
				}
				catch (json::type_error ex)
				{
					std::cerr << ex.what() << std::endl << opcode << std::endl;
					Assert::Fail(L"Failed json get");
				}

				//Check both paths for opcodes with conditionals
				if (cycles.size() > 1) {
					cpu.set_flag(CPU::Z, 0);
					cpu.set_flag(CPU::N, 0);
					cpu.set_flag(CPU::H, 0);
					cpu.set_flag(CPU::C, 0);

					start = gameboy->get_t_cycle_count();
					gameboy->tick_other_components();
					cpu.execute_opcode(i);
					end = gameboy->get_t_cycle_count();
					
					if (end - start == cycles[0]) {
						Assert::AreEqual(cycles[0], end - start, message.str().c_str());
						cycles.erase(cycles.begin());
					}
					else {
						Assert::AreEqual(cycles[1], end - start, message.str().c_str());
						cycles.erase(cycles.begin() + 1);
					}
					cpu.set_flag(CPU::Z, 1);
					cpu.set_flag(CPU::N, 1);
					cpu.set_flag(CPU::H, 1);
					cpu.set_flag(CPU::C, 1);
				}

				start = gameboy->get_t_cycle_count();
				gameboy->tick_other_components();
				cpu.execute_opcode(i);
				end = gameboy->get_t_cycle_count();

				Assert::AreEqual(cycles[0], end - start, message.str().c_str());
			}
		}

		TEST_METHOD(cb_opcode_timings)
		{
			json optables_json;
			std::ifstream file("..\\..\\paperGB_Tests\\Opcodes.json", std::ifstream::binary);

			try
			{
				optables_json = json::parse(file);
			}
			catch (json::parse_error& ex)
			{
				std::cerr << "parse error at byte " << ex.byte << std::endl;
				Assert::Fail(L"Failed to parse Opcodes.json");
			}

			Cartridge* cart = new Cartridge();
			GB* gameboy = new GB(*cart);
			CPU cpu = CPU(gameboy);
			int start = 0;
			int end = 0;
			std::stringstream opcode_stream;
			std::wstringstream message;
			std::vector<int> cycles;


			for (int i = 0; i <= 0xFF; i++) {
				opcode_stream.clear();
				opcode_stream.str("");
				opcode_stream << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << i;
				std::string opcode = opcode_stream.str();

				message.clear();
				message.str(L"");
				message << L"CB Opcode: " << std::wstring(opcode.begin(), opcode.end());

				try
				{
					cycles = optables_json["cbprefixed"][opcode]["cycles"].get<std::vector<int>>();
				}
				catch (json::type_error ex)
				{
					std::cerr << ex.what() << std::endl << opcode << std::endl;
					Assert::Fail(L"Failed json get");
				}

				start = gameboy->get_t_cycle_count();
				//One extra cycle for reading 0xCB prefix
				gameboy->tick_other_components();
				gameboy->tick_other_components();
				cpu.execute_CB_opcode(i);
				end = gameboy->get_t_cycle_count();

				Assert::AreEqual(cycles[0], end - start, message.str().c_str());
			}
		}
	};
}
