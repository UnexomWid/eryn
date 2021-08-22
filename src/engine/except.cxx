#include "engine.hxx"

Eryn::InternalException::InternalException(string msg, string fn, int ln) :
  message(msg), function(fn), line(ln) { }

const char* Eryn::InternalException::what() const noexcept {
    return message.c_str();
}

Eryn::CompilationException::CompilationException(const char* file, const char* msg, const char* description) :
  file(file), msg(msg), description(description) {
    message = "Compilation error in '";

    message += file;
    message += "'\n";
    message += msg;
    message += " (";
    message += description;
    message += ")";
}

Eryn::CompilationException::CompilationException(const char* file, const char* msg, const char* description, const Chunk& chunk) :
  file(file), msg(msg), description(description), chunk(chunk) {
    message = "Compilation error in '";

    message += file;
    message += "'\n";
    message += msg;
    message += " at ";
    message += std::to_string(chunk.line);
    message += ":";
    message += std::to_string(chunk.column);
    message += " (";
    message += description;
    message += ")\n";

    message += chunk.data;
    message += "\n";

    for(size_t i = 0; i < chunk.index; ++i) {
        message += "-";
    }
    message += "^\n";
}

const char* Eryn::CompilationException::what() const noexcept {
    return message.c_str();
}