#include "arg_parser.h"
#include "huffman_coding.h"

int main(int argc, char** argv) {
    ArgParser arg_parser;
    arg_parser.SetOptionalField("-c");
    arg_parser.SetOptionalField("-d");
    arg_parser.SetOptionalField("-h");

    try {
        arg_parser.Parse(argc, argv);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 111;
    }
    if (arg_parser.Empty()) {
        std::cerr << "At least one field must be added" << std::endl;
        return 111;
    }
    std::string field = arg_parser.GetField();
    if (field == "-c") {
        std::string output_filename = arg_parser.GetArgument(field, 0);
        BitWriter<std::ofstream> bit_writer(output_filename);
        BitString global_bit_string(0, 0);
        for (size_t i = 1; i < arg_parser.GetCountOfArguments(field); ++i) {
            std::string input_filename = arg_parser.GetArgument(field, i);
            BitReader<std::ifstream> bit_reader(input_filename);

            HuffmanCoding<std::ifstream, std::ofstream> huffman_code;

            bool is_file_last = (i + 1 == arg_parser.GetCountOfArguments(field));
            huffman_code.EncodeFile(bit_reader, bit_writer, global_bit_string, input_filename, is_file_last);
        }
    } else if (field == "-d") {
        std::string input_filename = arg_parser.GetArgument(field, 0);
        BitReader<std::ifstream> bit_reader(input_filename);
        HuffmanCoding<std::ifstream, std::ofstream> huffman_code;
        BitString global_bit_string(0, 0);
        while (not huffman_code.DecodeFile(bit_reader, global_bit_string)) {
        }
    } else {
        arg_parser.PrintHelp();
    }
    return 0;
}
