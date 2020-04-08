#ifndef ERYN_EXCEPT_RENDERING_HXX_GUARD
#define ERYN_EXCEPT_RENDERING_HXX_GUARD

#include <string>
#include <exception>

class RenderingException : public std::exception {
    private:
        const char* message;
    public:
        RenderingException(RenderingException &&e);
        RenderingException(const RenderingException &e);
        RenderingException(const char* msg, const char* description);;
        ~RenderingException();

        const char* what() const override;

        RenderingException& operator=(const RenderingException &e);
};

#endif