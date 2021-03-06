#include "compilation.hxx"

#include "../def/warnings.dxx"
#include "../common/str.hxx"

#include "../../lib/buffer.hxx"
#include "../../lib/remem.hxx"

CompilationException::CompilationException(const CompilationException& e) {
    message = strDup(e.message);
}

CompilationException::CompilationException(CompilationException&& e) {
    message = e.message;
    e.message = nullptr;
}

CompilationException::~CompilationException() {
    if(message != nullptr)
        re::free((char*) message);
}

CompilationException::CompilationException(const char* file, const char* msg, const char* description) {
    std::string buffer("Compilation error in '");

    buffer += file;
    buffer += "'\n";
    buffer += msg;
    buffer += " (";
    buffer += description;
    buffer += ")";

    message = strDup(buffer.c_str());
}

CompilationException::CompilationException(const char* file, const char* msg, const char* description, size_t line, size_t column, const uint8_t* chunk, size_t chunkIndex, size_t chunkSize) {
    std::string buffer("Compilation error in '");

    buffer += file;
    buffer += "'\n";
    buffer += msg;
    buffer += " at ";
    buffer += std::to_string(line);
    buffer += ":";
    buffer += std::to_string(column);
    buffer += " (";
    buffer += description;
    buffer += ")\n";

    if(chunk != nullptr && chunk != 0) {
        buffer += std::string(chunk, chunk + chunkSize);
        buffer += "\n";

        for(size_t i = 0; i < chunkIndex; ++i)
            buffer += "-";
        buffer += "^\n";
    }

    message = strDup(buffer.c_str());
}

const char* CompilationException::what() const noexcept {
    return message;
}

CompilationException& CompilationException::operator=(const CompilationException& e) {
    if(this == &e)
        return *this;

    if(message != nullptr)
        re::free((char*) message);

    message = strDup(e.message);

    return *this;
}