#include "rendering.hxx"

#include "../def/warnings.dxx"
#include "../common/str.hxx"

#include "../../lib/buffer.hxx"
#include "../../lib/remem.hxx"

RenderingException::RenderingException(const RenderingException& e) {
    message = strDup(e.message);
}

RenderingException::RenderingException(RenderingException&& e) {
    message = e.message;
    e.message = nullptr;
}

RenderingException::~RenderingException() {
    if(message != nullptr)
        re::free((char*) message);
}

RenderingException::RenderingException(const char* msg, const char* description) {
    std::string buffer(msg);

    buffer += " (";
    buffer += description;
    buffer += ")\n";

    message = strDup(buffer.c_str());
}

RenderingException::RenderingException(const char* msg, const char* description, const uint8_t* token, size_t tokenSize) {
    std::string buffer(msg);

    buffer += " (";
    buffer += description;
    buffer += ")\n";
    buffer += std::string(reinterpret_cast<const char*>(token), tokenSize);
    buffer += "\n^\n";

    message = strDup(buffer.c_str());
}

const char* RenderingException::what() const noexcept {
    return message;
}

RenderingException& RenderingException::operator=(const RenderingException& e) {
    if(this == &e)
        return *this;

    if(message != nullptr)
        re::free((char*) message);

    message = strDup(e.message);

    return *this;
}