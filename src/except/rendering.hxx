#ifndef ERYN_EXCEPT_RENDERING_HXX_GUARD
#define ERYN_EXCEPT_RENDERING_HXX_GUARD

#include <string>
#include <exception>

class RenderingException : public std::exception {
    private:
        const char* message;
    public:
        RenderingException(RenderingException&& e);
        RenderingException(const RenderingException& e);
        RenderingException(const char* msg, const char* description);
        RenderingException(const char* msg, const char* description, const uint8_t* token, size_t tokenSize);
        ~RenderingException();

        const char* what() const noexcept override;

        RenderingException& operator=(const RenderingException& e);
};

#endif