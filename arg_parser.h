#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class ArgumentException : public std::exception {
public:
    explicit ArgumentException(const std::string& message) : message_(message) {
    }
    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

class ArgParser {
public:
    ArgParser() {
    }
    void Parse(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            std::string field = std::string{argv[i]};
            if (not is_field_[field]) {
                throw ArgumentException(field + " must be a field");
            }
            while (i + 1 < argc and not is_field_[std::string{argv[i + 1]}]) {
                arguments_[field].push_back(std::string{argv[i + 1]});
                ++i;
            }
        }
        for (const auto& field : positional_fields_) {
            if (arguments_.find(field) == arguments_.end()) {
                throw ArgumentException("Positional field " + field + "was not specified");
            }
        }
    }
    std::string GetField() {
        return arguments_.begin()->first;
    }
    std::string GetArgument(std::string field, size_t position) {
        return arguments_[field][position];
    }
    size_t GetCountOfArguments(std::string field) {
        return arguments_[field].size();
    }
    void SetPositionalField(std::string name) {
        is_field_[name] = true;
        positional_fields_.push_back(name);
    }
    void SetOptionalField(std::string name) {
        is_field_[name] = true;
    }
    bool Empty() {
        return arguments_.empty();
    }
    void PrintHelp() {
        std::cerr << "Usage:" << '\n';
        std::cerr << "\t./archiver [option] file1 [files]:" << '\n';
        std::cerr << "Options:" << '\n';
        std::cerr << "\t -c \t\t encoding files" << '\n';
        std::cerr << "\t -d \t\t decoding files" << '\n';
        std::cerr << "\t -h \t\t print help" << '\n';
    }

private:
    std::unordered_map<std::string, std::vector<std::string>> arguments_;
    std::unordered_map<std::string, bool> is_field_;
    std::vector<std::string> positional_fields_;
};
