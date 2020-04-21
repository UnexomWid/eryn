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
        CompilationException(const char* file, const char* msg, const char* description);
        CompilationException(const char* file, const char* msg, const char* description, size_t line, size_t column, const uint8_t* chunk, size_t chunkIndex, size_t chunkSize);
        ~CompilationException();

        const char* what() const noexcept override;

        CompilationException& operator=(const CompilationException &e);
};

#endif