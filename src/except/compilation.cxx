#include "compilation.hxx"

CompilationException::CompilationException(const CompilationException &e) {
    message = strdup(e.message);
}

CompilationException::CompilationException(CompilationException &&e) {
    message = e.message;
    e.message = nullptr;
}

CompilationException::~CompilationException() {
    if(message != nullptr)
        free((char*) message);
}

CompilationException::CompilationException(const char* msg, const char* description) {
    std::string buffer(msg);
    buffer += "(";
    buffer += description;
    buffer += ")";
    message = strdup(buffer.c_str());
}

CompilationException::CompilationException(const char* msg, const char* description, size_t line, size_t column, const uint8_t* chunk, size_t chunkIndex, size_t chunkSize) {
    std::string buffer(msg);
    buffer.reserve(buffer.size() + 256);

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

    message = strdup(buffer.c_str());
}

const char* CompilationException::what() const {
    return message;
}

CompilationException& CompilationException::operator=(const CompilationException &e) {
    if(this == &e)
        return *this;

    if(message != nullptr)
        free((char*) message);

    message = strdup(e.message);

    return *this;
}