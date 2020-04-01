#ifndef ERYN_EXCEPT_COMPILATION_HXX_GUARD
#define ERYN_EXCEPT_COMPILATION_HXX_GUARD

#include <string>
#include <exception>

class CompilationException : public std::exception {
    private:
        const char* message;
    public:
        CompilationException(CompilationException &&e);
        CompilationException(const CompilationException &e);
        CompilationException(const char* msg, const char* description, size_t line, size_t column, const uint8_t* chunk, size_t chunkIndex);
        ~CompilationException();

        const char* what() const override;

        CompilationException& operator=(const CompilationException &e);
};

#endif