#include "engine.hxx"

Eryn::InternalException::InternalException(string msg, string fn, int ln)
  : message(msg), function(fn), line(ln) { }

const char* Eryn::InternalException::what() const noexcept {
    return message.c_str();
}

Eryn::CompilationException::CompilationException(const char* file, const char* msg, const char* description)
  : file(file), msg(msg), description(description) {
    message = "Compilation error in '";

    message += file;
    message += "'\n";
    message += msg;
    message += " (";
    message += description;
    message += ")";
}

Eryn::CompilationException::CompilationException(const char* file, const char* msg, const char* description, const Chunk& chunk)
  : file(file), msg(msg), description(description), chunk(chunk) {
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

Eryn::RenderingException::RenderingException(const char* msg, const char* description)
  : msg(msg), description(description), token(nullptr, 0) {
    message = "Rendering error\n";

    message += msg;
    message += " (";
    message += description;
    message += ")\n";
}

Eryn::RenderingException::RenderingException(const char* msg, const char* description, const char* path)
  : msg(msg), description(description), path(path), token(nullptr, 0) {
    message = "Rendering error in '";
    message += path;
    message += "'\n";

    message += msg;
    message += " (";
    message += description;
    message += ")";
}

Eryn::RenderingException::RenderingException(const char* msg, const char* description, ConstBuffer token)
  : msg(msg), description(description), token(token) {
    message = "Rendering error\n";

    message += msg;
    message += " (";
    message += description;
    message += ")\n";

    message += std::string(reinterpret_cast<const char*>(token.data), token.size);
    message += "\n^\n";
}

Eryn::RenderingException::RenderingException(const char* msg, const char* description, const char* path, ConstBuffer token)
  : msg(msg), description(description), path(path), token(token) {
    message = "Rendering error in '";
    message += path;
    message += "'\n";

    message += msg;
    message += " (";
    message += description;
    message += ")\n";

    message += std::string(reinterpret_cast<const char*>(token.data), token.size);
    message += "\n^\n";
}

const char* Eryn::RenderingException::what() const noexcept {
    return message.c_str();
}