#include "rendering.hxx"

RenderingException::RenderingException(const RenderingException &e) {
    message = strdup(e.message);
}

RenderingException::RenderingException(RenderingException &&e) {
    message = e.message;
    e.message = nullptr;
}

RenderingException::~RenderingException() {
    if(message != nullptr)
        free((char*) message);
}

RenderingException::RenderingException(const char* msg, const char* description) {
    std::string buffer(msg);
    buffer += " (";
    buffer += description;
    buffer += ")";
    message = strdup(buffer.c_str());
}

const char* RenderingException::what() const {
    return message;
}

RenderingException& RenderingException::operator=(const RenderingException &e) {
    if(this == &e)
        return *this;

    if(message != nullptr)
        free((char*) message);

    message = strdup(e.message);

    return *this;
}