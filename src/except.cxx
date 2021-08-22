#include "engine.hxx"

Eryn::InternalException::InternalException(string msg, string fn, int ln) :
    message(msg), function(fn), line(ln) { }

const char* Eryn::InternalException::what() const noexcept {
    return message.c_str();
}