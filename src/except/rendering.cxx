#include "rendering.hxx"
#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"

RenderingException::RenderingException(const RenderingException &e) {
    message = qstrdup(e.message);
}

RenderingException::RenderingException(RenderingException &&e) {
    message = e.message;
    e.message = nullptr;
}

RenderingException::~RenderingException() {
    if(message != nullptr)
        qfree((char*) message);
}

RenderingException::RenderingException(const char* msg, const char* description) {
    std::string buffer(msg);
    buffer += " (";
    buffer += description;
    buffer += ")\n";
    message = qstrdup(buffer.c_str());
}

const char* RenderingException::what() const {
    return message;
}

RenderingException& RenderingException::operator=(const RenderingException &e) {
    if(this == &e)
        return *this;

    if(message != nullptr)
        qfree((char*) message);

    message = qstrdup(e.message);

    return *this;
}